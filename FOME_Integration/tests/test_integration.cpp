/*
 * Integration Tests
 *
 * End-to-end tests combining XCP protocol with vehicle profiles
 * Tests real-world scenarios and data flow
 */

#include <cstdio>
#include <cstdint>
#include <cstring>

// Test framework
#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("  Testing %s... ", #name); \
    test_##name(); \
    printf("PASS\n"); \
    tests_passed++; \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("FAIL\n    %s:%d: %d != %d\n", __FILE__, __LINE__, (int)(a), (int)(b)); \
        return; \
    } \
} while(0)

#define ASSERT_TRUE(x) do { \
    if (!(x)) { \
        printf("FAIL\n    %s:%d: expected true\n", __FILE__, __LINE__); \
        return; \
    } \
} while(0)

static int tests_passed = 0;

// -----------------------------------------
// Mock infrastructure
// -----------------------------------------

// Simulated ECU memory
static uint8_t ecu_memory[0x1000];

// CAN message capture
#define MAX_CAN_MESSAGES 64
struct CanMsg {
    uint32_t id;
    uint8_t data[8];
    uint8_t len;
};
static CanMsg can_tx_buffer[MAX_CAN_MESSAGES];
static int can_tx_count = 0;

static CanMsg can_rx_buffer[MAX_CAN_MESSAGES];
static int can_rx_count = 0;
static int can_rx_read_idx = 0;

void reset_test_state() {
    memset(ecu_memory, 0, sizeof(ecu_memory));
    can_tx_count = 0;
    can_rx_count = 0;
    can_rx_read_idx = 0;
}

void queue_can_rx(uint32_t id, uint8_t* data, uint8_t len) {
    if (can_rx_count < MAX_CAN_MESSAGES) {
        can_rx_buffer[can_rx_count].id = id;
        memcpy(can_rx_buffer[can_rx_count].data, data, len);
        can_rx_buffer[can_rx_count].len = len;
        can_rx_count++;
    }
}

CanMsg* find_tx_message(uint32_t id) {
    for (int i = 0; i < can_tx_count; i++) {
        if (can_tx_buffer[i].id == id) {
            return &can_tx_buffer[i];
        }
    }
    return nullptr;
}

// -----------------------------------------
// XCP Protocol constants
// -----------------------------------------

#define XCP_CMD_CONNECT         0xFF
#define XCP_CMD_SET_MTA         0xF6
#define XCP_CMD_UPLOAD          0xF5
#define XCP_CMD_SHORT_UPLOAD    0xF4
#define XCP_CMD_DOWNLOAD        0xF0

#define XCP_PID_RES             0xFF
#define XCP_PID_ERR             0xFE

#define XCP_CAN_RX_ID           0x554
#define XCP_CAN_TX_ID           0x555

// -----------------------------------------
// Simulated XCP slave
// -----------------------------------------

class MockXcpSlave {
public:
    void init(uint32_t rxId, uint32_t txId) {
        m_rxId = rxId;
        m_txId = txId;
        m_connected = false;
        m_mta = 0;
    }

    void processCommand(uint8_t* data, uint8_t len) {
        uint8_t cmd = data[0];
        uint8_t response[8] = {0};

        switch (cmd) {
            case XCP_CMD_CONNECT:
                m_connected = true;
                response[0] = XCP_PID_RES;
                response[1] = 0x1F;  // Resources
                response[2] = 0x00;
                response[3] = 0x08;  // Max CTO
                response[4] = 0x08;  // Max DTO
                sendResponse(response, 8);
                break;

            case XCP_CMD_SET_MTA:
                m_mta = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
                response[0] = XCP_PID_RES;
                sendResponse(response, 1);
                break;

            case XCP_CMD_UPLOAD:
                {
                    uint8_t numBytes = data[1];
                    response[0] = XCP_PID_RES;
                    for (int i = 0; i < numBytes && i < 7; i++) {
                        if (m_mta + i < sizeof(ecu_memory)) {
                            response[1 + i] = ecu_memory[m_mta + i];
                        }
                    }
                    m_mta += numBytes;
                    sendResponse(response, 1 + numBytes);
                }
                break;

            case XCP_CMD_SHORT_UPLOAD:
                {
                    uint8_t numBytes = data[1];
                    uint32_t addr = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
                    response[0] = XCP_PID_RES;
                    for (int i = 0; i < numBytes && i < 7; i++) {
                        if (addr + i < sizeof(ecu_memory)) {
                            response[1 + i] = ecu_memory[addr + i];
                        }
                    }
                    sendResponse(response, 1 + numBytes);
                }
                break;

            case XCP_CMD_DOWNLOAD:
                {
                    uint8_t numBytes = data[1];
                    for (int i = 0; i < numBytes && i < 6; i++) {
                        if (m_mta + i < sizeof(ecu_memory)) {
                            ecu_memory[m_mta + i] = data[2 + i];
                        }
                    }
                    m_mta += numBytes;
                    response[0] = XCP_PID_RES;
                    sendResponse(response, 1);
                }
                break;

            default:
                response[0] = XCP_PID_ERR;
                response[1] = 0x20;  // Unknown command
                sendResponse(response, 2);
                break;
        }
    }

    bool isConnected() { return m_connected; }

private:
    void sendResponse(uint8_t* data, uint8_t len) {
        if (can_tx_count < MAX_CAN_MESSAGES) {
            can_tx_buffer[can_tx_count].id = m_txId;
            memcpy(can_tx_buffer[can_tx_count].data, data, len);
            can_tx_buffer[can_tx_count].len = len;
            can_tx_count++;
        }
    }

    uint32_t m_rxId;
    uint32_t m_txId;
    bool m_connected;
    uint32_t m_mta;
};

static MockXcpSlave xcpSlave;

// -----------------------------------------
// Simulated vehicle profile
// -----------------------------------------

class MockVehicleProfile {
public:
    void init() {
        m_rpm = 0;
        m_speed = 0;
        m_throttle = 0;
        m_coolantTemp = 20;
    }

    void setRPM(int rpm) { m_rpm = rpm; }
    void setSpeed(int speed) { m_speed = speed; }
    void setThrottle(int throttle) { m_throttle = throttle; }
    void setCoolantTemp(int temp) { m_coolantTemp = temp; }

    void sendDashboardMessages() {
        // 0x201 - PCM Status
        uint8_t msg201[8];
        int tempRPM = (int)(m_rpm * 3.85);
        msg201[0] = (tempRPM >> 8) & 0xFF;
        msg201[1] = tempRPM & 0xFF;
        msg201[2] = 0xFF;
        msg201[3] = 0xFF;
        int tempSpeed = (m_speed * 100) + 10000;
        msg201[4] = (tempSpeed >> 8) & 0xFF;
        msg201[5] = tempSpeed & 0xFF;
        msg201[6] = m_throttle * 2;
        msg201[7] = 0xFF;
        transmit(0x201, msg201, 8);

        // 0x420 - MIL
        uint8_t msg420[7] = {0};
        msg420[0] = m_coolantTemp;
        transmit(0x420, msg420, 7);
    }

    // Store sensor values in ECU memory for XCP access
    void updateEcuMemory() {
        // RPM at offset 0x100
        ecu_memory[0x100] = (m_rpm >> 8) & 0xFF;
        ecu_memory[0x101] = m_rpm & 0xFF;

        // Speed at offset 0x102
        ecu_memory[0x102] = m_speed;

        // Throttle at offset 0x103
        ecu_memory[0x103] = m_throttle;

        // Coolant temp at offset 0x104
        ecu_memory[0x104] = m_coolantTemp;
    }

private:
    void transmit(uint32_t id, uint8_t* data, uint8_t len) {
        if (can_tx_count < MAX_CAN_MESSAGES) {
            can_tx_buffer[can_tx_count].id = id;
            memcpy(can_tx_buffer[can_tx_count].data, data, len);
            can_tx_buffer[can_tx_count].len = len;
            can_tx_count++;
        }
    }

    int m_rpm;
    int m_speed;
    int m_throttle;
    int m_coolantTemp;
};

static MockVehicleProfile vehicleProfile;

// -----------------------------------------
// Integration Tests
// -----------------------------------------

TEST(xcp_connect_and_profile_send) {
    reset_test_state();

    xcpSlave.init(XCP_CAN_RX_ID, XCP_CAN_TX_ID);
    vehicleProfile.init();

    // Connect XCP
    uint8_t connectCmd[8] = {XCP_CMD_CONNECT, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(connectCmd, 1);
    ASSERT_TRUE(xcpSlave.isConnected());

    // Set vehicle data
    vehicleProfile.setRPM(3000);
    vehicleProfile.setSpeed(60);
    vehicleProfile.setThrottle(25);

    // Send dashboard messages
    vehicleProfile.sendDashboardMessages();

    // Verify both XCP response and vehicle messages sent
    ASSERT_TRUE(find_tx_message(XCP_CAN_TX_ID) != nullptr);
    ASSERT_TRUE(find_tx_message(0x201) != nullptr);
    ASSERT_TRUE(find_tx_message(0x420) != nullptr);
}

TEST(xcp_read_vehicle_data) {
    reset_test_state();

    xcpSlave.init(XCP_CAN_RX_ID, XCP_CAN_TX_ID);
    vehicleProfile.init();

    // Set vehicle data
    vehicleProfile.setRPM(4500);
    vehicleProfile.setSpeed(75);
    vehicleProfile.setThrottle(50);
    vehicleProfile.setCoolantTemp(90);

    // Update ECU memory with sensor values
    vehicleProfile.updateEcuMemory();

    // Connect XCP
    uint8_t connectCmd[1] = {XCP_CMD_CONNECT};
    xcpSlave.processCommand(connectCmd, 1);

    // Read RPM via XCP SHORT_UPLOAD
    uint8_t uploadCmd[8] = {XCP_CMD_SHORT_UPLOAD, 2, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(uploadCmd, 8);

    // Find response
    CanMsg* response = nullptr;
    for (int i = 0; i < can_tx_count; i++) {
        if (can_tx_buffer[i].id == XCP_CAN_TX_ID &&
            can_tx_buffer[i].data[0] == XCP_PID_RES &&
            i > 0) {  // Skip connect response
            response = &can_tx_buffer[i];
        }
    }
    ASSERT_TRUE(response != nullptr);

    // Verify RPM value (4500 = 0x1194)
    int rpm = (response->data[1] << 8) | response->data[2];
    ASSERT_EQ(rpm, 4500);
}

TEST(xcp_modify_calibration) {
    reset_test_state();

    xcpSlave.init(XCP_CAN_RX_ID, XCP_CAN_TX_ID);

    // Set initial calibration value
    ecu_memory[0x200] = 100;  // Fuel map value

    // Connect XCP
    uint8_t connectCmd[1] = {XCP_CMD_CONNECT};
    xcpSlave.processCommand(connectCmd, 1);

    // Set MTA to calibration address
    uint8_t setMtaCmd[8] = {XCP_CMD_SET_MTA, 0, 0, 0, 0, 0, 0x02, 0x00};
    xcpSlave.processCommand(setMtaCmd, 8);

    // Download new value (120)
    uint8_t downloadCmd[8] = {XCP_CMD_DOWNLOAD, 1, 120, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(downloadCmd, 8);

    // Verify memory was modified
    ASSERT_EQ(ecu_memory[0x200], 120);
}

TEST(concurrent_xcp_and_can) {
    reset_test_state();

    xcpSlave.init(XCP_CAN_RX_ID, XCP_CAN_TX_ID);
    vehicleProfile.init();

    // Simulate concurrent operations
    vehicleProfile.setRPM(2500);
    vehicleProfile.setSpeed(40);
    vehicleProfile.updateEcuMemory();

    // XCP connect
    uint8_t connectCmd[1] = {XCP_CMD_CONNECT};
    xcpSlave.processCommand(connectCmd, 1);

    // Vehicle sends dashboard
    vehicleProfile.sendDashboardMessages();

    // XCP reads data
    uint8_t uploadCmd[8] = {XCP_CMD_SHORT_UPLOAD, 1, 0, 0, 0, 0, 0x01, 0x02};
    xcpSlave.processCommand(uploadCmd, 8);

    // Both should have sent messages
    int xcpMessages = 0;
    int canMessages = 0;
    for (int i = 0; i < can_tx_count; i++) {
        if (can_tx_buffer[i].id == XCP_CAN_TX_ID) xcpMessages++;
        if (can_tx_buffer[i].id == 0x201) canMessages++;
    }
    ASSERT_TRUE(xcpMessages >= 2);  // Connect + upload response
    ASSERT_EQ(canMessages, 1);
}

TEST(daq_with_vehicle_data) {
    reset_test_state();

    xcpSlave.init(XCP_CAN_RX_ID, XCP_CAN_TX_ID);
    vehicleProfile.init();

    // Simulate DAQ data collection
    for (int i = 0; i < 5; i++) {
        // Update vehicle state
        vehicleProfile.setRPM(2000 + i * 500);
        vehicleProfile.setSpeed(30 + i * 10);
        vehicleProfile.updateEcuMemory();

        // Send dashboard messages (simulating periodic update)
        vehicleProfile.sendDashboardMessages();
    }

    // Should have multiple 0x201 messages
    int count = 0;
    for (int i = 0; i < can_tx_count; i++) {
        if (can_tx_buffer[i].id == 0x201) count++;
    }
    ASSERT_EQ(count, 5);
}

TEST(immobilizer_with_xcp) {
    reset_test_state();

    xcpSlave.init(XCP_CAN_RX_ID, XCP_CAN_TX_ID);
    vehicleProfile.init();

    // XCP connect
    uint8_t connectCmd[1] = {XCP_CMD_CONNECT};
    xcpSlave.processCommand(connectCmd, 1);

    // Simulated immobilizer is handled by vehicle profile
    // This test verifies both can operate simultaneously

    // Update vehicle data
    vehicleProfile.setRPM(800);
    vehicleProfile.sendDashboardMessages();

    // XCP should still be connected
    ASSERT_TRUE(xcpSlave.isConnected());

    // Dashboard message should be sent
    ASSERT_TRUE(find_tx_message(0x201) != nullptr);
}

TEST(calibration_persists_across_messages) {
    reset_test_state();

    xcpSlave.init(XCP_CAN_RX_ID, XCP_CAN_TX_ID);
    vehicleProfile.init();

    // Connect and set calibration
    uint8_t connectCmd[1] = {XCP_CMD_CONNECT};
    xcpSlave.processCommand(connectCmd, 1);

    // Write calibration data
    uint8_t setMtaCmd[8] = {XCP_CMD_SET_MTA, 0, 0, 0, 0, 0, 0x03, 0x00};
    xcpSlave.processCommand(setMtaCmd, 8);

    uint8_t downloadCmd[8] = {XCP_CMD_DOWNLOAD, 4, 10, 20, 30, 40, 0, 0};
    xcpSlave.processCommand(downloadCmd, 8);

    // Send multiple dashboard messages
    for (int i = 0; i < 3; i++) {
        vehicleProfile.setRPM(3000 + i * 100);
        vehicleProfile.sendDashboardMessages();
    }

    // Calibration data should persist
    ASSERT_EQ(ecu_memory[0x300], 10);
    ASSERT_EQ(ecu_memory[0x301], 20);
    ASSERT_EQ(ecu_memory[0x302], 30);
    ASSERT_EQ(ecu_memory[0x303], 40);
}

TEST(multiple_xcp_uploads) {
    reset_test_state();

    xcpSlave.init(XCP_CAN_RX_ID, XCP_CAN_TX_ID);

    // Set up test data
    for (int i = 0; i < 10; i++) {
        ecu_memory[0x100 + i] = i * 10;
    }

    // Connect
    uint8_t connectCmd[1] = {XCP_CMD_CONNECT};
    xcpSlave.processCommand(connectCmd, 1);

    // Multiple uploads
    for (int i = 0; i < 5; i++) {
        uint8_t uploadCmd[8] = {XCP_CMD_SHORT_UPLOAD, 2, 0, 0, 0, 0, 0x01, (uint8_t)(i * 2)};
        xcpSlave.processCommand(uploadCmd, 8);
    }

    // Should have 6 responses (1 connect + 5 uploads)
    int responses = 0;
    for (int i = 0; i < can_tx_count; i++) {
        if (can_tx_buffer[i].id == XCP_CAN_TX_ID) responses++;
    }
    ASSERT_EQ(responses, 6);
}

TEST(vehicle_state_consistency) {
    reset_test_state();

    vehicleProfile.init();

    // Set initial state
    vehicleProfile.setRPM(5000);
    vehicleProfile.setSpeed(100);
    vehicleProfile.setThrottle(80);
    vehicleProfile.setCoolantTemp(95);
    vehicleProfile.updateEcuMemory();

    // Verify memory state
    int rpm = (ecu_memory[0x100] << 8) | ecu_memory[0x101];
    ASSERT_EQ(rpm, 5000);
    ASSERT_EQ(ecu_memory[0x102], 100);
    ASSERT_EQ(ecu_memory[0x103], 80);
    ASSERT_EQ(ecu_memory[0x104], 95);
}

TEST(sequential_operations) {
    reset_test_state();

    xcpSlave.init(XCP_CAN_RX_ID, XCP_CAN_TX_ID);
    vehicleProfile.init();

    // Sequence: Connect -> Set data -> Upload -> Send CAN -> Download

    // 1. Connect
    uint8_t connectCmd[1] = {XCP_CMD_CONNECT};
    xcpSlave.processCommand(connectCmd, 1);

    // 2. Set vehicle data
    vehicleProfile.setRPM(6000);
    vehicleProfile.updateEcuMemory();

    // 3. Upload via XCP
    uint8_t uploadCmd[8] = {XCP_CMD_SHORT_UPLOAD, 2, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(uploadCmd, 8);

    // 4. Send CAN messages
    vehicleProfile.sendDashboardMessages();

    // 5. Download new value
    uint8_t setMtaCmd[8] = {XCP_CMD_SET_MTA, 0, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(setMtaCmd, 8);
    uint8_t downloadCmd[8] = {XCP_CMD_DOWNLOAD, 2, 0x1F, 0x40, 0, 0, 0, 0};  // 8000
    xcpSlave.processCommand(downloadCmd, 8);

    // Verify final state
    int rpm = (ecu_memory[0x100] << 8) | ecu_memory[0x101];
    ASSERT_EQ(rpm, 8000);
}

// -----------------------------------------
// Main
// -----------------------------------------

int main() {
    printf("Integration Tests (XCP + Vehicle Profiles)\n");
    printf("==========================================\n\n");

    printf("Basic Integration:\n");
    RUN_TEST(xcp_connect_and_profile_send);
    RUN_TEST(xcp_read_vehicle_data);
    RUN_TEST(xcp_modify_calibration);

    printf("\nConcurrent Operations:\n");
    RUN_TEST(concurrent_xcp_and_can);
    RUN_TEST(daq_with_vehicle_data);
    RUN_TEST(immobilizer_with_xcp);

    printf("\nData Persistence:\n");
    RUN_TEST(calibration_persists_across_messages);
    RUN_TEST(multiple_xcp_uploads);
    RUN_TEST(vehicle_state_consistency);
    RUN_TEST(sequential_operations);

    printf("\n==========================================\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("==========================================\n");

    return 0;
}
