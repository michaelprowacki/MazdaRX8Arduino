/*
 * XCP Protocol Unit Tests
 *
 * Tests for XCP slave implementation
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

// Mock implementations for platform functions
static uint8_t mock_memory[0x1000];
static uint8_t last_tx_data[8];
static uint8_t last_tx_length;
static uint32_t last_tx_id;
static uint32_t mock_timestamp = 0;

extern "C" {
    void xcpCanTransmit(uint32_t canId, uint8_t* data, uint8_t length) {
        last_tx_id = canId;
        memcpy(last_tx_data, data, length);
        last_tx_length = length;
    }

    uint8_t xcpReadByte(uint32_t address) {
        if (address < 0x1000) {
            return mock_memory[address];
        }
        return 0xFF;
    }

    void xcpWriteByte(uint32_t address, uint8_t value) {
        if (address < 0x1000) {
            mock_memory[address] = value;
        }
    }

    uint32_t xcpGetTimestamp() {
        return mock_timestamp++;
    }

    // Flash function stubs (not used in basic protocol tests)
    bool xcpFlashErase(uint32_t address, uint32_t length) {
        (void)address;
        (void)length;
        return true;
    }

    bool xcpFlashWrite(uint32_t address, uint8_t* data, uint32_t length) {
        (void)address;
        (void)data;
        (void)length;
        return true;
    }

    bool xcpFlashVerify(uint32_t address, uint8_t* data, uint32_t length) {
        (void)address;
        (void)data;
        (void)length;
        return true;
    }
}

#include "../xcp/xcp_protocol.h"
#include "../xcp/xcp_protocol.cpp"

// Test counters
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

#define ASSERT_TRUE(x) do { \
    if (!(x)) { \
        printf("FAILED: %s is false\n", #x); \
        return; \
    } \
} while(0)

// Reset test state
void reset_test_state() {
    memset(mock_memory, 0, sizeof(mock_memory));
    memset(last_tx_data, 0, sizeof(last_tx_data));
    last_tx_length = 0;
    last_tx_id = 0;
    mock_timestamp = 0;
    xcpSlave.init(0x554, 0x555);
}

// Test: Connect command
TEST(xcp_connect) {
    reset_test_state();

    uint8_t cmd[8] = {XCP_CMD_CONNECT, 0x00, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(cmd, 2);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
    ASSERT_TRUE(last_tx_length >= 8);
}

// Test: Disconnect command
TEST(xcp_disconnect) {
    reset_test_state();

    // First connect
    uint8_t connect[8] = {XCP_CMD_CONNECT, 0x00, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(connect, 2);

    // Then disconnect
    uint8_t disconnect[8] = {XCP_CMD_DISCONNECT, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(disconnect, 1);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
}

// Test: Command without connection should fail
TEST(xcp_no_connection_error) {
    reset_test_state();

    // Try to get status without connecting
    uint8_t cmd[8] = {XCP_CMD_GET_STATUS, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(cmd, 1);

    ASSERT_EQ(last_tx_data[0], XCP_PID_ERR);
    ASSERT_EQ(last_tx_data[1], XCP_ERR_SEQUENCE);
}

// Test: Set MTA and Upload
TEST(xcp_set_mta_upload) {
    reset_test_state();

    // Setup test data
    mock_memory[0x100] = 0xAA;
    mock_memory[0x101] = 0xBB;
    mock_memory[0x102] = 0xCC;
    mock_memory[0x103] = 0xDD;

    // Connect
    uint8_t connect[8] = {XCP_CMD_CONNECT, 0x00, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(connect, 2);

    // Set MTA to 0x100
    uint8_t setMta[8] = {XCP_CMD_SET_MTA, 0, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(setMta, 8);
    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);

    // Upload 4 bytes
    uint8_t upload[8] = {XCP_CMD_UPLOAD, 4, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(upload, 2);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
    ASSERT_EQ(last_tx_data[1], 0xAA);
    ASSERT_EQ(last_tx_data[2], 0xBB);
    ASSERT_EQ(last_tx_data[3], 0xCC);
    ASSERT_EQ(last_tx_data[4], 0xDD);
}

// Test: Short Upload
TEST(xcp_short_upload) {
    reset_test_state();

    // Setup test data
    mock_memory[0x200] = 0x12;
    mock_memory[0x201] = 0x34;

    // Connect
    uint8_t connect[8] = {XCP_CMD_CONNECT, 0x00, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(connect, 2);

    // Short upload from address 0x200
    uint8_t shortUpload[8] = {XCP_CMD_SHORT_UPLOAD, 2, 0, 0, 0, 0, 0x02, 0x00};
    xcpSlave.processCommand(shortUpload, 8);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
    ASSERT_EQ(last_tx_data[1], 0x12);
    ASSERT_EQ(last_tx_data[2], 0x34);
}

// Test: Download
TEST(xcp_download) {
    reset_test_state();

    // Connect
    uint8_t connect[8] = {XCP_CMD_CONNECT, 0x00, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(connect, 2);

    // Set MTA to 0x300
    uint8_t setMta[8] = {XCP_CMD_SET_MTA, 0, 0, 0, 0, 0, 0x03, 0x00};
    xcpSlave.processCommand(setMta, 8);

    // Download 3 bytes
    uint8_t download[8] = {XCP_CMD_DOWNLOAD, 3, 0x11, 0x22, 0x33, 0, 0, 0};
    xcpSlave.processCommand(download, 5);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
    ASSERT_EQ(mock_memory[0x300], 0x11);
    ASSERT_EQ(mock_memory[0x301], 0x22);
    ASSERT_EQ(mock_memory[0x302], 0x33);
}

// Test: Get Status
TEST(xcp_get_status) {
    reset_test_state();

    // Connect
    uint8_t connect[8] = {XCP_CMD_CONNECT, 0x00, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(connect, 2);

    // Get status
    uint8_t status[8] = {XCP_CMD_GET_STATUS, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(status, 1);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
    ASSERT_EQ(last_tx_length, 6);
}

// Test: Unknown command error
TEST(xcp_unknown_command) {
    reset_test_state();

    // Connect
    uint8_t connect[8] = {XCP_CMD_CONNECT, 0x00, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(connect, 2);

    // Send invalid command
    uint8_t invalid[8] = {0x01, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(invalid, 1);

    ASSERT_EQ(last_tx_data[0], XCP_PID_ERR);
    ASSERT_EQ(last_tx_data[1], XCP_ERR_CMD_UNKNOWN);
}

// Test: DAQ allocation
TEST(xcp_daq_alloc) {
    reset_test_state();

    // Connect
    uint8_t connect[8] = {XCP_CMD_CONNECT, 0x00, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(connect, 2);

    // Free DAQ
    uint8_t freeDaq[8] = {XCP_CMD_FREE_DAQ, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(freeDaq, 1);
    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);

    // Alloc 2 DAQ lists
    uint8_t allocDaq[8] = {XCP_CMD_ALLOC_DAQ, 0, 0, 2, 0, 0, 0, 0};
    xcpSlave.processCommand(allocDaq, 4);
    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);

    // Alloc 3 ODTs for DAQ list 0
    uint8_t allocOdt[8] = {XCP_CMD_ALLOC_ODT, 0, 0, 0, 3, 0, 0, 0};
    xcpSlave.processCommand(allocOdt, 5);
    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
}

// Test: Get DAQ processor info
TEST(xcp_get_daq_processor_info) {
    reset_test_state();

    // Connect
    uint8_t connect[8] = {XCP_CMD_CONNECT, 0x00, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(connect, 2);

    // Get DAQ processor info
    uint8_t getDaqInfo[8] = {XCP_CMD_GET_DAQ_PROCESSOR_INFO, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(getDaqInfo, 1);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
    ASSERT_EQ(last_tx_length, 8);
}

// Test: Get DAQ clock
TEST(xcp_get_daq_clock) {
    reset_test_state();
    mock_timestamp = 12345;

    // Connect
    uint8_t connect[8] = {XCP_CMD_CONNECT, 0x00, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(connect, 2);

    // Get DAQ clock
    uint8_t getClock[8] = {XCP_CMD_GET_DAQ_CLOCK, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(getClock, 1);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
    // Timestamp in bytes 4-7
    uint32_t timestamp = ((uint32_t)last_tx_data[4] << 24) |
                         ((uint32_t)last_tx_data[5] << 16) |
                         ((uint32_t)last_tx_data[6] << 8) |
                         last_tx_data[7];
    ASSERT_EQ(timestamp, 12345);
}

int main() {
    printf("XCP Protocol Unit Tests\n");
    printf("========================\n\n");

    RUN_TEST(xcp_connect);
    RUN_TEST(xcp_disconnect);
    RUN_TEST(xcp_no_connection_error);
    RUN_TEST(xcp_set_mta_upload);
    RUN_TEST(xcp_short_upload);
    RUN_TEST(xcp_download);
    RUN_TEST(xcp_get_status);
    RUN_TEST(xcp_unknown_command);
    RUN_TEST(xcp_daq_alloc);
    RUN_TEST(xcp_get_daq_processor_info);
    RUN_TEST(xcp_get_daq_clock);

    printf("\n========================\n");
    printf("Tests: %d/%d passed\n", tests_passed, tests_run);

    return (tests_passed == tests_run) ? 0 : 1;
}
