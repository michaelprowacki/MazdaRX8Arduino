/**
 * @file idle_control.cpp
 * @brief Closed-loop idle control implementation
 *
 * @author Created for Phase 5+ Haltech-style features
 * @date 2025-11-16
 */

#include "idle_control.h"
#include <Arduino.h>

namespace IdleControl {

// ============================================================================
// CONFIGURATION
// ============================================================================

static IdleConfig config = {
    .mode = MODE_CLOSED_LOOP,
    .target_rpm_warm = 800,         // 800 RPM warm idle (rotary typical)
    .target_rpm_cold = 1200,        // 1200 RPM cold idle
    .warm_temp = 800,               // 80°C = warm engine
    .ac_rpm_adder = 100,            // +100 RPM with AC on
    .ps_rpm_adder = 50,             // +50 RPM with PS load
    .dashpot_delay = 30,            // 3.0 second dashpot
    .pid_kp = 0.05f,                // Proportional gain
    .pid_ki = 0.01f,                // Integral gain
    .pid_kd = 0.002f                // Derivative gain
};

static IdleStatus status = {
    .target_rpm = 800,
    .actual_rpm = 0,
    .rpm_error = 0,
    .iac_position = 50,             // Start at 50%
    .ac_active = false,
    .ps_load = false,
    .dashpot_active = false,
    .idle_active = false
};

// PID state
static float pid_integral = 0.0f;
static float pid_previous_error = 0.0f;
static uint32_t last_update_ms = 0;

// Dashpot state
static uint32_t throttle_close_time = 0;
static bool throttle_was_open = false;

// ============================================================================
// INITIALIZATION
// ============================================================================

void init() {
    Serial.println("[IDLE] Idle control initialized");
    Serial.printf("[IDLE] Mode: %s\n",
                 config.mode == MODE_DISABLED ? "Disabled" :
                 config.mode == MODE_OPEN_LOOP ? "Open Loop" : "Closed Loop");
    Serial.printf("[IDLE] Target RPM: %d (warm), %d (cold)\n",
                 config.target_rpm_warm, config.target_rpm_cold);
    Serial.printf("[IDLE] PID gains: Kp=%.3f, Ki=%.3f, Kd=%.3f\n",
                 config.pid_kp, config.pid_ki, config.pid_kd);

    last_update_ms = millis();
}

// ============================================================================
// MAIN UPDATE
// ============================================================================

uint8_t update(uint16_t rpm, uint8_t throttle, int16_t coolant_temp,
               bool ac_on, bool ps_load) {

    status.actual_rpm = rpm;
    status.ac_active = ac_on;
    status.ps_load = ps_load;

    // Check if at idle
    status.idle_active = isIdle(throttle, rpm);

    if (config.mode == MODE_DISABLED) {
        return 0;  // No IAC control
    }

    // Calculate target RPM
    status.target_rpm = calculateTargetRPM(coolant_temp);

    // Add compensation for AC
    if (ac_on) {
        status.target_rpm += config.ac_rpm_adder;
    }

    // Add compensation for power steering
    if (ps_load) {
        status.target_rpm += config.ps_rpm_adder;
    }

    // Calculate error
    status.rpm_error = status.target_rpm - rpm;

    if (config.mode == MODE_OPEN_LOOP) {
        // Open loop: use fixed IAC position
        // (status.iac_position set manually via setIACPosition)
        return status.iac_position;
    }

    // Closed loop: PID control
    uint32_t now = millis();
    float dt = (now - last_update_ms) / 1000.0f;  // Convert to seconds
    last_update_ms = now;

    if (status.idle_active) {
        // At idle: use PID control
        int16_t pid_adjustment = calculatePID(status.rpm_error, dt);
        status.iac_position = constrain(status.iac_position + pid_adjustment, 0, 100);
    } else {
        // Not at idle: reset PID, apply dashpot
        resetPID();

        // Dashpot: prevents stalling on sudden throttle close
        uint8_t dashpot = calculateDashpot(throttle, rpm);
        if (dashpot > 0) {
            status.iac_position = dashpot;
            status.dashpot_active = true;
        } else {
            status.dashpot_active = false;
            // Return to base position when off-idle
            status.iac_position = 30;  // Base off-idle position
        }
    }

    return status.iac_position;
}

// ============================================================================
// TARGET RPM CALCULATION
// ============================================================================

uint16_t calculateTargetRPM(int16_t coolant_temp) {
    // Warm engine: use target_rpm_warm
    if (coolant_temp >= config.warm_temp) {
        return config.target_rpm_warm;
    }

    // Cold engine: interpolate between cold and warm RPM
    // Example curve:
    // -20°C: 1500 RPM
    // 0°C: 1200 RPM
    // 40°C: 1000 RPM
    // 80°C+: 800 RPM

    int16_t cold_rpm = config.target_rpm_cold;
    int16_t warm_rpm = config.target_rpm_warm;

    // Linear interpolation based on temp
    // 0°C = cold_rpm, warm_temp = warm_rpm
    if (coolant_temp <= 0) {
        // Below 0°C: even higher idle
        int16_t extra = (abs(coolant_temp) * 15) / 10;  // +1.5 RPM per degree below 0
        return cold_rpm + extra;
    }

    // 0 to warm_temp: interpolate
    int16_t rpm = cold_rpm -
                 ((cold_rpm - warm_rpm) * coolant_temp) / config.warm_temp;

    return (uint16_t)rpm;
}

// ============================================================================
// PID CONTROL
// ============================================================================

int16_t calculatePID(int16_t error, float dt) {
    if (dt <= 0) {
        return 0;
    }

    // Proportional term
    float p_term = config.pid_kp * error;

    // Integral term
    pid_integral += error * dt;
    // Anti-windup: clamp integrator
    pid_integral = constrain(pid_integral, -500.0f, 500.0f);
    float i_term = config.pid_ki * pid_integral;

    // Derivative term
    float d_term = config.pid_kd * (error - pid_previous_error) / dt;
    pid_previous_error = error;

    // Total PID output
    float output = p_term + i_term + d_term;

    return (int16_t)output;
}

void resetPID() {
    pid_integral = 0.0f;
    pid_previous_error = 0.0f;
}

// ============================================================================
// DASHPOT CONTROL
// ============================================================================

uint8_t calculateDashpot(uint8_t throttle, uint16_t rpm) {
    // Dashpot: holds IAC open briefly when throttle closes suddenly
    // This prevents stalling during quick throttle closures

    const uint8_t THROTTLE_THRESHOLD = 5;  // Consider closed below 5%

    // Detect throttle closing
    if (throttle > THROTTLE_THRESHOLD) {
        throttle_was_open = true;
        return 0;  // No dashpot while throttle open
    }

    // Throttle is closed
    if (throttle_was_open) {
        // Just closed - activate dashpot
        throttle_close_time = millis();
        throttle_was_open = false;
    }

    // Check if dashpot should be active
    uint32_t time_since_close = millis() - throttle_close_time;
    uint32_t dashpot_duration_ms = config.dashpot_delay * 100;  // Convert to ms

    if (time_since_close < dashpot_duration_ms) {
        // Dashpot active: hold IAC open
        // Ramp down over dashpot duration
        uint8_t dashpot_position = 70 -
                                  (time_since_close * 40) / dashpot_duration_ms;
        return dashpot_position;
    }

    return 0;  // Dashpot expired
}

// ============================================================================
// IDLE DETECTION
// ============================================================================

bool isIdle(uint8_t throttle, uint16_t rpm) {
    // Consider at idle if:
    // 1. Throttle nearly closed (< 5%)
    // 2. RPM near idle range (500-1500 RPM)

    const uint8_t THROTTLE_THRESHOLD = 5;
    const uint16_t MIN_IDLE_RPM = 500;
    const uint16_t MAX_IDLE_RPM = 1500;

    bool throttle_closed = (throttle < THROTTLE_THRESHOLD);
    bool rpm_in_range = (rpm >= MIN_IDLE_RPM && rpm <= MAX_IDLE_RPM);

    return throttle_closed && rpm_in_range;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

const IdleStatus* getStatus() {
    return &status;
}

void configure(const IdleConfig& new_config) {
    config = new_config;
    Serial.println("[IDLE] Idle control configuration updated");
    init();
}

void setMode(ControlMode mode) {
    config.mode = mode;
    Serial.printf("[IDLE] Mode set to: %s\n",
                 mode == MODE_DISABLED ? "Disabled" :
                 mode == MODE_OPEN_LOOP ? "Open Loop" : "Closed Loop");

    if (mode == MODE_CLOSED_LOOP) {
        resetPID();
    }
}

void setTargetRPM(uint16_t rpm) {
    config.target_rpm_warm = rpm;
    Serial.printf("[IDLE] Target RPM (warm) set to: %d\n", rpm);
}

void setIACPosition(uint8_t position) {
    status.iac_position = constrain(position, 0, 100);
    Serial.printf("[IDLE] IAC position set to: %d%%\n", position);
}

} // namespace IdleControl
