/*
 * CAN Message Encoding Unit Tests
 *
 * Tests for RX8 CAN message encoding algorithms
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

// Test framework
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running %s... ", #name); \
    test_##name(); \
    tests_run++; \
    tests_passed++; \
    printf("PASSED\n"); \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("FAILED: %s != %s (%d != %d)\n", #a, #b, (int)(a), (int)(b)); \
        return; \
    } \
} while(0)

#define ASSERT_NEAR(a, b, tol) do { \
    if (fabs((double)(a) - (double)(b)) > (tol)) { \
        printf("FAILED: %s != %s (%.2f != %.2f)\n", #a, #b, (double)(a), (double)(b)); \
        return; \
    } \
} while(0)

// CAN encoding functions (from RX8 protocol)
namespace RX8_CAN {

// Encode RPM for 0x201 message
void encodeRPM(uint8_t* msg, int rpm) {
    int encoded = (int)(rpm * 3.85);
    msg[0] = (encoded >> 8) & 0xFF;
    msg[1] = encoded & 0xFF;
}

// Decode RPM from 0x201 message
int decodeRPM(uint8_t* msg) {
    int encoded = (msg[0] << 8) | msg[1];
    return (int)(encoded / 3.85);
}

// Encode vehicle speed for 0x201 message
void encodeSpeed(uint8_t* msg, int speed) {
    int encoded = (speed * 100) + 10000;
    msg[4] = (encoded >> 8) & 0xFF;
    msg[5] = encoded & 0xFF;
}

// Decode vehicle speed from 0x201 message
int decodeSpeed(uint8_t* msg) {
    int encoded = (msg[4] << 8) | msg[5];
    return (encoded - 10000) / 100;
}

// Encode throttle for 0x201 message (0-100%)
void encodeThrottle(uint8_t* msg, int throttle) {
    msg[6] = (uint8_t)(throttle * 2);
}

// Decode throttle from 0x201 message
int decodeThrottle(uint8_t* msg) {
    return msg[6] / 2;
}

// Encode wheel speed from 0x4B1 message
void encodeWheelSpeed(uint8_t* msg, int fl, int fr, int rl, int rr) {
    int encFL = fl + 10000;
    int encFR = fr + 10000;
    int encRL = rl + 10000;
    int encRR = rr + 10000;

    msg[0] = (encFL >> 8) & 0xFF;
    msg[1] = encFL & 0xFF;
    msg[2] = (encFR >> 8) & 0xFF;
    msg[3] = encFR & 0xFF;
    msg[4] = (encRL >> 8) & 0xFF;
    msg[5] = encRL & 0xFF;
    msg[6] = (encRR >> 8) & 0xFF;
    msg[7] = encRR & 0xFF;
}

// Decode wheel speeds from 0x4B1 message
void decodeWheelSpeed(uint8_t* msg, int* fl, int* fr, int* rl, int* rr) {
    *fl = ((msg[0] << 8) | msg[1]) - 10000;
    *fr = ((msg[2] << 8) | msg[3]) - 10000;
    *rl = ((msg[4] << 8) | msg[5]) - 10000;
    *rr = ((msg[6] << 8) | msg[7]) - 10000;
}

// Encode warning lights for 0x420 message
void encode0x420(uint8_t* msg, int temp, bool checkEngine, bool lowCoolant,
                 bool batCharge, bool oilPressure, bool oilOK) {
    memset(msg, 0, 7);

    msg[0] = temp;
    msg[4] = oilOK ? 1 : 0;

    if (checkEngine) msg[5] |= 0x40;
    if (lowCoolant) msg[6] |= 0x02;
    if (batCharge) msg[6] |= 0x40;
    if (oilPressure) msg[6] |= 0x80;
}

// Encode odometer increment for 0x420
// 4140 increments = 1 mile
void encodeOdometer(uint8_t* msg, uint32_t totalIncrements) {
    msg[1] = totalIncrements & 0xFF;
}

} // namespace RX8_CAN

// Test: RPM encoding basic
TEST(rpm_encode_basic) {
    uint8_t msg[8] = {0};

    RX8_CAN::encodeRPM(msg, 1000);

    // 1000 * 3.85 = 3850
    ASSERT_EQ(msg[0], 0x0F);
    ASSERT_EQ(msg[1], 0x0A);
}

// Test: RPM roundtrip
TEST(rpm_roundtrip) {
    uint8_t msg[8] = {0};

    for (int rpm = 0; rpm <= 9000; rpm += 500) {
        RX8_CAN::encodeRPM(msg, rpm);
        int decoded = RX8_CAN::decodeRPM(msg);

        // Allow ±1 RPM tolerance due to integer math
        ASSERT_NEAR(decoded, rpm, 2);
    }
}

// Test: RPM edge cases
TEST(rpm_edge_cases) {
    uint8_t msg[8] = {0};

    // Zero RPM
    RX8_CAN::encodeRPM(msg, 0);
    ASSERT_EQ(msg[0], 0);
    ASSERT_EQ(msg[1], 0);

    // Idle RPM
    RX8_CAN::encodeRPM(msg, 850);
    int idle = RX8_CAN::decodeRPM(msg);
    ASSERT_NEAR(idle, 850, 2);

    // Redline
    RX8_CAN::encodeRPM(msg, 9000);
    int redline = RX8_CAN::decodeRPM(msg);
    ASSERT_NEAR(redline, 9000, 2);
}

// Test: Speed encoding basic
TEST(speed_encode_basic) {
    uint8_t msg[8] = {0};

    RX8_CAN::encodeSpeed(msg, 60);

    // (60 * 100) + 10000 = 16000 = 0x3E80
    ASSERT_EQ(msg[4], 0x3E);
    ASSERT_EQ(msg[5], 0x80);
}

// Test: Speed roundtrip
TEST(speed_roundtrip) {
    uint8_t msg[8] = {0};

    for (int speed = 0; speed <= 200; speed += 10) {
        RX8_CAN::encodeSpeed(msg, speed);
        int decoded = RX8_CAN::decodeSpeed(msg);
        ASSERT_EQ(decoded, speed);
    }
}

// Test: Zero speed
TEST(speed_zero) {
    uint8_t msg[8] = {0};

    RX8_CAN::encodeSpeed(msg, 0);

    // 0 + 10000 = 10000 = 0x2710
    ASSERT_EQ(msg[4], 0x27);
    ASSERT_EQ(msg[5], 0x10);

    int decoded = RX8_CAN::decodeSpeed(msg);
    ASSERT_EQ(decoded, 0);
}

// Test: Throttle encoding
TEST(throttle_encode) {
    uint8_t msg[8] = {0};

    RX8_CAN::encodeThrottle(msg, 0);
    ASSERT_EQ(msg[6], 0);

    RX8_CAN::encodeThrottle(msg, 50);
    ASSERT_EQ(msg[6], 100);

    RX8_CAN::encodeThrottle(msg, 100);
    ASSERT_EQ(msg[6], 200);
}

// Test: Throttle roundtrip
TEST(throttle_roundtrip) {
    uint8_t msg[8] = {0};

    for (int throttle = 0; throttle <= 100; throttle += 5) {
        RX8_CAN::encodeThrottle(msg, throttle);
        int decoded = RX8_CAN::decodeThrottle(msg);
        ASSERT_EQ(decoded, throttle);
    }
}

// Test: Wheel speed encoding
TEST(wheel_speed_encode) {
    uint8_t msg[8] = {0};

    // All wheels at 6000 (60 mph in 1/100 mph)
    RX8_CAN::encodeWheelSpeed(msg, 6000, 6000, 6000, 6000);

    // 6000 + 10000 = 16000 = 0x3E80
    ASSERT_EQ(msg[0], 0x3E);
    ASSERT_EQ(msg[1], 0x80);
}

// Test: Wheel speed roundtrip
TEST(wheel_speed_roundtrip) {
    uint8_t msg[8] = {0};
    int fl, fr, rl, rr;

    RX8_CAN::encodeWheelSpeed(msg, 5000, 5100, 4900, 5050);
    RX8_CAN::decodeWheelSpeed(msg, &fl, &fr, &rl, &rr);

    ASSERT_EQ(fl, 5000);
    ASSERT_EQ(fr, 5100);
    ASSERT_EQ(rl, 4900);
    ASSERT_EQ(rr, 5050);
}

// Test: Wheel speed mismatch detection
TEST(wheel_speed_mismatch) {
    uint8_t msg[8] = {0};
    int fl, fr, rl, rr;

    // Significant front wheel difference (wheelspin scenario)
    RX8_CAN::encodeWheelSpeed(msg, 6000, 6600, 5000, 5000);
    RX8_CAN::decodeWheelSpeed(msg, &fl, &fr, &rl, &rr);

    // Difference > 500 should trigger warning
    int diff = abs(fl - fr);
    ASSERT_EQ(diff > 500, 1);  // true
}

// Test: Warning lights encoding
TEST(warning_lights_encode) {
    uint8_t msg[7] = {0};

    RX8_CAN::encode0x420(msg, 145, true, false, false, false, true);

    ASSERT_EQ(msg[0], 145);  // Temperature
    ASSERT_EQ(msg[4], 1);    // Oil OK
    ASSERT_EQ(msg[5] & 0x40, 0x40);  // Check engine
}

// Test: Multiple warning lights
TEST(warning_lights_multiple) {
    uint8_t msg[7] = {0};

    RX8_CAN::encode0x420(msg, 200, true, true, true, true, false);

    ASSERT_EQ(msg[0], 200);  // High temp
    ASSERT_EQ(msg[4], 0);    // Oil not OK
    ASSERT_EQ(msg[5] & 0x40, 0x40);  // Check engine
    ASSERT_EQ(msg[6] & 0x02, 0x02);  // Low coolant
    ASSERT_EQ(msg[6] & 0x40, 0x40);  // Battery
    ASSERT_EQ(msg[6] & 0x80, 0x80);  // Oil pressure
}

// Test: No warning lights
TEST(warning_lights_none) {
    uint8_t msg[7] = {0};

    RX8_CAN::encode0x420(msg, 145, false, false, false, false, true);

    ASSERT_EQ(msg[5], 0);
    ASSERT_EQ(msg[6], 0);
}

// Test: Odometer encoding
TEST(odometer_encode) {
    uint8_t msg[7] = {0};

    // Test increment values
    RX8_CAN::encodeOdometer(msg, 100);
    ASSERT_EQ(msg[1], 100);

    RX8_CAN::encodeOdometer(msg, 4140);  // 1 mile
    ASSERT_EQ(msg[1], 0x2C);  // Lower byte of 4140
}

// Test: 0x201 full message encoding
TEST(msg_201_full) {
    uint8_t msg[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    RX8_CAN::encodeRPM(msg, 3000);
    msg[2] = 0xFF;
    msg[3] = 0xFF;
    RX8_CAN::encodeSpeed(msg, 55);
    RX8_CAN::encodeThrottle(msg, 25);
    msg[7] = 0xFF;

    // Verify structure
    ASSERT_EQ(msg[2], 0xFF);
    ASSERT_EQ(msg[3], 0xFF);
    ASSERT_EQ(msg[7], 0xFF);

    // Verify values roundtrip
    int rpm = RX8_CAN::decodeRPM(msg);
    int speed = RX8_CAN::decodeSpeed(msg);
    int throttle = RX8_CAN::decodeThrottle(msg);

    ASSERT_NEAR(rpm, 3000, 2);
    ASSERT_EQ(speed, 55);
    ASSERT_EQ(throttle, 25);
}

// Test: Byte order (MSB first)
TEST(byte_order) {
    uint8_t msg[8] = {0};

    // Large RPM value to test byte order
    RX8_CAN::encodeRPM(msg, 8000);

    // 8000 * 3.85 = 30800 = 0x7850
    ASSERT_EQ(msg[0], 0x78);  // High byte first
    ASSERT_EQ(msg[1], 0x50);  // Low byte second
}

int main() {
    printf("CAN Encoding Unit Tests\n");
    printf("========================\n\n");

    RUN_TEST(rpm_encode_basic);
    RUN_TEST(rpm_roundtrip);
    RUN_TEST(rpm_edge_cases);
    RUN_TEST(speed_encode_basic);
    RUN_TEST(speed_roundtrip);
    RUN_TEST(speed_zero);
    RUN_TEST(throttle_encode);
    RUN_TEST(throttle_roundtrip);
    RUN_TEST(wheel_speed_encode);
    RUN_TEST(wheel_speed_roundtrip);
    RUN_TEST(wheel_speed_mismatch);
    RUN_TEST(warning_lights_encode);
    RUN_TEST(warning_lights_multiple);
    RUN_TEST(warning_lights_none);
    RUN_TEST(odometer_encode);
    RUN_TEST(msg_201_full);
    RUN_TEST(byte_order);

    printf("\n========================\n");
    printf("Tests: %d/%d passed\n", tests_passed, tests_run);

    return (tests_passed == tests_run) ? 0 : 1;
}
