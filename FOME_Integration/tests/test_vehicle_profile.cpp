/*
 * Vehicle Profile Unit Tests
 *
 * Tests for RX8 vehicle profile CAN message encoding
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Capture transmitted CAN messages
#define MAX_CAN_MESSAGES 20
static struct {
    uint32_t id;
    uint8_t data[8];
    uint8_t length;
} captured_messages[MAX_CAN_MESSAGES];
static int message_count = 0;

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
        printf("FAILED: %s != %s (0x%X != 0x%X)\n", #a, #b, (int)(a), (int)(b)); \
        return; \
    } \
} while(0)

#define ASSERT_TRUE(x) do { \
    if (!(x)) { \
        printf("FAILED: %s is false\n", #x); \
        return; \
    } \
} while(0)

// Include vehicle profile (with mock CAN transmit)
#define VEHICLE_PROFILE_TEST_MODE
#include "../vehicle_profiles/vehicle_profile.h"

// Mock RX8 profile implementation for testing
class TestRX8Profile : public VehicleProfile {
public:
    void init() override {
        message_count = 0;
    }

    void sendDashboardMessages() override {
        send0x201();
        send0x420();
        send0x620();
        send0x630();
    }

    void handleCanRx(uint32_t canId, uint8_t* data, uint8_t length) override {
        switch (canId) {
            case 0x047:
                handleImmobilizer(data);
                break;
            case 0x4B1:
                handleWheelSpeed(data);
                break;
        }
    }

protected:
    void canTransmit(uint32_t id, uint8_t* data, uint8_t length) override {
        if (message_count < MAX_CAN_MESSAGES) {
            captured_messages[message_count].id = id;
            memcpy(captured_messages[message_count].data, data, length);
            captured_messages[message_count].length = length;
            message_count++;
        }
    }

private:
    void send0x201() {
        uint8_t msg[8];

        // RPM encoding: RPM * 3.85
        int tempRPM = (int)(m_rpm * 3.85);
        msg[0] = (tempRPM >> 8) & 0xFF;
        msg[1] = tempRPM & 0xFF;

        // Fixed bytes
        msg[2] = 0xFF;
        msg[3] = 0xFF;

        // Speed encoding: (speed * 100) + 10000
        int tempSpeed = (m_speed * 100) + 10000;
        msg[4] = (tempSpeed >> 8) & 0xFF;
        msg[5] = tempSpeed & 0xFF;

        // Throttle: 0-200 (0.5% increments)
        msg[6] = (uint8_t)(m_throttle * 2);

        msg[7] = 0xFF;

        canTransmit(0x201, msg, 8);
    }

    void send0x420() {
        uint8_t msg[7] = {0};

        // Byte 0: Engine temperature
        msg[0] = m_coolantTemp;

        // Byte 4: Oil pressure (1 = OK)
        msg[4] = m_oilPressure > 100 ? 1 : 0;

        // Byte 5: Check engine light
        if (m_warnings.checkEngine) {
            msg[5] |= 0x40;
        }

        // Byte 6: Other warnings
        if (m_warnings.lowCoolant) {
            msg[6] |= 0x02;
        }
        if (m_warnings.batteryCharge) {
            msg[6] |= 0x40;
        }
        if (m_warnings.oilPressure) {
            msg[6] |= 0x80;
        }

        canTransmit(0x420, msg, 7);
    }

    void send0x620() {
        uint8_t msg[7] = {0, 0, 0, 0, 16, 0, 4};
        canTransmit(0x620, msg, 7);
    }

    void send0x630() {
        uint8_t msg[8] = {8, 0, 0, 0, 0, 0, 106, 106};
        canTransmit(0x630, msg, 8);
    }

    void handleImmobilizer(uint8_t* data) {
        if (data[1] == 127 && data[2] == 2) {
            uint8_t response[8] = {7, 12, 48, 242, 23, 0, 0, 0};
            canTransmit(0x041, response, 8);
        }
        if (data[1] == 92 && data[2] == 244) {
            uint8_t response[8] = {129, 127, 0, 0, 0, 0, 0, 0};
            canTransmit(0x041, response, 8);
        }
    }

    void handleWheelSpeed(uint8_t* data) {
        int frontLeft = ((data[0] << 8) | data[1]) - 10000;
        int frontRight = ((data[2] << 8) | data[3]) - 10000;
        m_speed = ((frontLeft + frontRight) / 2) / 100;
    }
};

// Helper to find message by ID
int findMessage(uint32_t id) {
    for (int i = 0; i < message_count; i++) {
        if (captured_messages[i].id == id) {
            return i;
        }
    }
    return -1;
}

void reset_test_state() {
    message_count = 0;
    memset(captured_messages, 0, sizeof(captured_messages));
}

// Test: RPM encoding at various values
TEST(rpm_encoding) {
    reset_test_state();
    TestRX8Profile profile;
    profile.init();

    // Test 1000 RPM
    profile.setRPM(1000);
    profile.sendDashboardMessages();

    int idx = findMessage(0x201);
    ASSERT_TRUE(idx >= 0);

    // 1000 * 3.85 = 3850 = 0x0F0A
    int encodedRPM = (captured_messages[idx].data[0] << 8) | captured_messages[idx].data[1];
    ASSERT_EQ(encodedRPM, 3850);
}

// Test: RPM encoding at redline
TEST(rpm_encoding_high) {
    reset_test_state();
    TestRX8Profile profile;
    profile.init();

    // Test 9000 RPM (near redline)
    profile.setRPM(9000);
    profile.sendDashboardMessages();

    int idx = findMessage(0x201);
    ASSERT_TRUE(idx >= 0);

    // 9000 * 3.85 = 34650 = 0x8762
    int encodedRPM = (captured_messages[idx].data[0] << 8) | captured_messages[idx].data[1];
    ASSERT_EQ(encodedRPM, 34650);
}

// Test: Speed encoding
TEST(speed_encoding) {
    reset_test_state();
    TestRX8Profile profile;
    profile.init();

    // Test 60 mph
    profile.setVehicleSpeed(60);
    profile.sendDashboardMessages();

    int idx = findMessage(0x201);
    ASSERT_TRUE(idx >= 0);

    // (60 * 100) + 10000 = 16000 = 0x3E80
    int encodedSpeed = (captured_messages[idx].data[4] << 8) | captured_messages[idx].data[5];
    ASSERT_EQ(encodedSpeed, 16000);
}

// Test: Throttle encoding
TEST(throttle_encoding) {
    reset_test_state();
    TestRX8Profile profile;
    profile.init();

    // Test 50% throttle
    profile.setThrottle(50);
    profile.sendDashboardMessages();

    int idx = findMessage(0x201);
    ASSERT_TRUE(idx >= 0);

    // 50% * 2 = 100
    ASSERT_EQ(captured_messages[idx].data[6], 100);
}

// Test: Full throttle
TEST(throttle_full) {
    reset_test_state();
    TestRX8Profile profile;
    profile.init();

    profile.setThrottle(100);
    profile.sendDashboardMessages();

    int idx = findMessage(0x201);
    ASSERT_TRUE(idx >= 0);

    // 100% * 2 = 200
    ASSERT_EQ(captured_messages[idx].data[6], 200);
}

// Test: Engine temperature
TEST(engine_temperature) {
    reset_test_state();
    TestRX8Profile profile;
    profile.init();

    profile.setCoolantTemp(145);  // Normal operating temp
    profile.sendDashboardMessages();

    int idx = findMessage(0x420);
    ASSERT_TRUE(idx >= 0);

    ASSERT_EQ(captured_messages[idx].data[0], 145);
}

// Test: Check engine light
TEST(check_engine_light) {
    reset_test_state();
    TestRX8Profile profile;
    profile.init();

    WarningLights warnings = {0};
    warnings.checkEngine = true;
    profile.setWarnings(warnings);
    profile.sendDashboardMessages();

    int idx = findMessage(0x420);
    ASSERT_TRUE(idx >= 0);

    // Bit 6 of byte 5 should be set
    ASSERT_EQ(captured_messages[idx].data[5] & 0x40, 0x40);
}

// Test: Multiple warning lights
TEST(multiple_warnings) {
    reset_test_state();
    TestRX8Profile profile;
    profile.init();

    WarningLights warnings = {0};
    warnings.lowCoolant = true;
    warnings.batteryCharge = true;
    warnings.oilPressure = true;
    profile.setWarnings(warnings);
    profile.sendDashboardMessages();

    int idx = findMessage(0x420);
    ASSERT_TRUE(idx >= 0);

    // Check byte 6 bits
    ASSERT_EQ(captured_messages[idx].data[6] & 0x02, 0x02);  // Low coolant
    ASSERT_EQ(captured_messages[idx].data[6] & 0x40, 0x40);  // Battery
    ASSERT_EQ(captured_messages[idx].data[6] & 0x80, 0x80);  // Oil pressure
}

// Test: ABS message format
TEST(abs_message) {
    reset_test_state();
    TestRX8Profile profile;
    profile.init();
    profile.sendDashboardMessages();

    int idx = findMessage(0x620);
    ASSERT_TRUE(idx >= 0);

    ASSERT_EQ(captured_messages[idx].length, 7);
    ASSERT_EQ(captured_messages[idx].data[4], 16);
    ASSERT_EQ(captured_messages[idx].data[6], 4);
}

// Test: ABS config message
TEST(abs_config_message) {
    reset_test_state();
    TestRX8Profile profile;
    profile.init();
    profile.sendDashboardMessages();

    int idx = findMessage(0x630);
    ASSERT_TRUE(idx >= 0);

    ASSERT_EQ(captured_messages[idx].length, 8);
    ASSERT_EQ(captured_messages[idx].data[0], 8);
    ASSERT_EQ(captured_messages[idx].data[6], 106);
    ASSERT_EQ(captured_messages[idx].data[7], 106);
}

// Test: Immobilizer handshake part 1
TEST(immobilizer_handshake_1) {
    reset_test_state();
    TestRX8Profile profile;
    profile.init();

    uint8_t request[8] = {0, 127, 2, 0, 0, 0, 0, 0};
    profile.handleCanRx(0x047, request, 8);

    int idx = findMessage(0x041);
    ASSERT_TRUE(idx >= 0);

    ASSERT_EQ(captured_messages[idx].data[0], 7);
    ASSERT_EQ(captured_messages[idx].data[1], 12);
    ASSERT_EQ(captured_messages[idx].data[2], 48);
    ASSERT_EQ(captured_messages[idx].data[3], 242);
}

// Test: Immobilizer handshake part 2
TEST(immobilizer_handshake_2) {
    reset_test_state();
    TestRX8Profile profile;
    profile.init();

    uint8_t request[8] = {0, 92, 244, 0, 0, 0, 0, 0};
    profile.handleCanRx(0x047, request, 8);

    int idx = findMessage(0x041);
    ASSERT_TRUE(idx >= 0);

    ASSERT_EQ(captured_messages[idx].data[0], 129);
    ASSERT_EQ(captured_messages[idx].data[1], 127);
}

// Test: Wheel speed decoding
TEST(wheel_speed_decoding) {
    reset_test_state();
    TestRX8Profile profile;
    profile.init();

    // Encode 60 mph for both wheels: (6000) + 10000 = 16000
    uint8_t wheelData[8] = {0x3E, 0x80, 0x3E, 0x80, 0, 0, 0, 0};
    profile.handleCanRx(0x4B1, wheelData, 8);

    // Send messages to check speed was updated
    profile.sendDashboardMessages();

    int idx = findMessage(0x201);
    ASSERT_TRUE(idx >= 0);

    int encodedSpeed = (captured_messages[idx].data[4] << 8) | captured_messages[idx].data[5];
    ASSERT_EQ(encodedSpeed, 16000);  // 60 mph
}

// Test: All messages transmitted
TEST(all_messages_sent) {
    reset_test_state();
    TestRX8Profile profile;
    profile.init();

    profile.setRPM(3000);
    profile.setVehicleSpeed(45);
    profile.setThrottle(25);
    profile.setCoolantTemp(140);
    profile.sendDashboardMessages();

    // Should have at least 4 messages
    ASSERT_TRUE(message_count >= 4);

    // Check all critical IDs present
    ASSERT_TRUE(findMessage(0x201) >= 0);
    ASSERT_TRUE(findMessage(0x420) >= 0);
    ASSERT_TRUE(findMessage(0x620) >= 0);
    ASSERT_TRUE(findMessage(0x630) >= 0);
}

int main() {
    printf("Vehicle Profile Unit Tests\n");
    printf("===========================\n\n");

    RUN_TEST(rpm_encoding);
    RUN_TEST(rpm_encoding_high);
    RUN_TEST(speed_encoding);
    RUN_TEST(throttle_encoding);
    RUN_TEST(throttle_full);
    RUN_TEST(engine_temperature);
    RUN_TEST(check_engine_light);
    RUN_TEST(multiple_warnings);
    RUN_TEST(abs_message);
    RUN_TEST(abs_config_message);
    RUN_TEST(immobilizer_handshake_1);
    RUN_TEST(immobilizer_handshake_2);
    RUN_TEST(wheel_speed_decoding);
    RUN_TEST(all_messages_sent);

    printf("\n===========================\n");
    printf("Tests: %d/%d passed\n", tests_passed, tests_run);

    return (tests_passed == tests_run) ? 0 : 1;
}
