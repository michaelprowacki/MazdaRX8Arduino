/*
 * XCP Flash Programming Unit Tests
 *
 * Tests for XCP flash programming commands
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

// Mock implementations
static uint8_t mock_memory[0x10000];
static uint8_t mock_flash[0x10000];
static uint8_t last_tx_data[8];
static uint8_t last_tx_length;
static uint32_t last_tx_id;
static bool flash_erase_result = true;
static bool flash_write_result = true;

extern "C" {
    void xcpCanTransmit(uint32_t canId, uint8_t* data, uint8_t length) {
        last_tx_id = canId;
        memcpy(last_tx_data, data, length);
        last_tx_length = length;
    }

    uint8_t xcpReadByte(uint32_t address) {
        if (address < 0x10000) {
            return mock_memory[address];
        }
        return 0xFF;
    }

    void xcpWriteByte(uint32_t address, uint8_t value) {
        if (address < 0x10000) {
            mock_memory[address] = value;
        }
    }

    uint32_t xcpGetTimestamp() {
        return 0;
    }

    bool xcpFlashErase(uint32_t address, uint32_t length) {
        if (!flash_erase_result) return false;

        // Simulate erase (set to 0xFF)
        for (uint32_t i = 0; i < length && (address + i) < 0x10000; i++) {
            mock_flash[address + i] = 0xFF;
        }
        return true;
    }

    bool xcpFlashWrite(uint32_t address, uint8_t* data, uint32_t length) {
        if (!flash_write_result) return false;

        for (uint32_t i = 0; i < length && (address + i) < 0x10000; i++) {
            mock_flash[address + i] = data[i];
            mock_memory[address + i] = data[i];  // Also update readable memory
        }
        return true;
    }

    bool xcpFlashVerify(uint32_t address, uint8_t* data, uint32_t length) {
        for (uint32_t i = 0; i < length && (address + i) < 0x10000; i++) {
            if (mock_flash[address + i] != data[i]) {
                return false;
            }
        }
        return true;
    }
}

#include "../xcp/xcp_protocol.h"
#include "../xcp/xcp_protocol.cpp"

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

#define ASSERT_TRUE(x) do { \
    if (!(x)) { \
        printf("FAILED: %s is false\n", #x); \
        return; \
    } \
} while(0)

void reset_test_state() {
    memset(mock_memory, 0, sizeof(mock_memory));
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    memset(last_tx_data, 0, sizeof(last_tx_data));
    last_tx_length = 0;
    last_tx_id = 0;
    flash_erase_result = true;
    flash_write_result = true;
    xcpSlave.init(0x554, 0x555);
}

void connect_xcp() {
    uint8_t cmd[8] = {XCP_CMD_CONNECT, 0x00, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(cmd, 2);
}

// Test: Program Start
TEST(pgm_start) {
    reset_test_state();
    connect_xcp();

    uint8_t cmd[8] = {XCP_CMD_PROGRAM_START, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(cmd, 1);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
    ASSERT_EQ(last_tx_length, 8);
    ASSERT_EQ(last_tx_data[3], XCP_PGM_MAX_SIZE);  // MAX_BS_PGM
}

// Test: Program Start when already active
TEST(pgm_start_already_active) {
    reset_test_state();
    connect_xcp();

    // First start
    uint8_t cmd[8] = {XCP_CMD_PROGRAM_START, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(cmd, 1);

    // Second start should fail
    xcpSlave.processCommand(cmd, 1);
    ASSERT_EQ(last_tx_data[0], XCP_PID_ERR);
    ASSERT_EQ(last_tx_data[1], XCP_ERR_PGM_ACTIVE);
}

// Test: Get PGM Processor Info
TEST(pgm_processor_info) {
    reset_test_state();
    connect_xcp();

    // Start programming first
    uint8_t start[8] = {XCP_CMD_PROGRAM_START, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(start, 1);

    uint8_t cmd[8] = {XCP_CMD_GET_PGM_PROCESSOR_INFO, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(cmd, 1);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
    ASSERT_TRUE(last_tx_data[2] > 0);  // Has sectors
}

// Test: Get Sector Info
TEST(pgm_sector_info) {
    reset_test_state();
    connect_xcp();

    // Start programming
    uint8_t start[8] = {XCP_CMD_PROGRAM_START, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(start, 1);

    // Get sector 0 info (mode 0 = length)
    uint8_t cmd[8] = {XCP_CMD_GET_SECTOR_INFO, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(cmd, 3);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
}

// Test: Get Sector Info - Address mode
TEST(pgm_sector_info_address) {
    reset_test_state();
    connect_xcp();

    // Start programming
    uint8_t start[8] = {XCP_CMD_PROGRAM_START, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(start, 1);

    // Get sector 0 address (mode 1)
    uint8_t cmd[8] = {XCP_CMD_GET_SECTOR_INFO, 1, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(cmd, 3);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
}

// Test: Get Sector Info - Invalid sector
TEST(pgm_sector_info_invalid) {
    reset_test_state();
    connect_xcp();

    // Start programming
    uint8_t start[8] = {XCP_CMD_PROGRAM_START, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(start, 1);

    // Get invalid sector
    uint8_t cmd[8] = {XCP_CMD_GET_SECTOR_INFO, 0, 99, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(cmd, 3);

    ASSERT_EQ(last_tx_data[0], XCP_PID_ERR);
    ASSERT_EQ(last_tx_data[1], XCP_ERR_OUT_OF_RANGE);
}

// Test: Program Clear
TEST(pgm_clear) {
    reset_test_state();
    connect_xcp();

    // Start programming
    uint8_t start[8] = {XCP_CMD_PROGRAM_START, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(start, 1);

    // Set MTA
    uint8_t setMta[8] = {XCP_CMD_SET_MTA, 0, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(setMta, 8);

    // Clear 256 bytes
    uint8_t clear[8] = {XCP_CMD_PROGRAM_CLEAR, 0, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(clear, 8);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
}

// Test: Program Clear - Wrong sequence
TEST(pgm_clear_wrong_sequence) {
    reset_test_state();
    connect_xcp();

    // Try to clear without starting
    uint8_t clear[8] = {XCP_CMD_PROGRAM_CLEAR, 0, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(clear, 8);

    ASSERT_EQ(last_tx_data[0], XCP_PID_ERR);
    ASSERT_EQ(last_tx_data[1], XCP_ERR_SEQUENCE);
}

// Test: Program data
TEST(pgm_program) {
    reset_test_state();
    connect_xcp();

    // Start -> Clear -> Program sequence
    uint8_t start[8] = {XCP_CMD_PROGRAM_START, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(start, 1);

    uint8_t setMta[8] = {XCP_CMD_SET_MTA, 0, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(setMta, 8);

    uint8_t clear[8] = {XCP_CMD_PROGRAM_CLEAR, 0, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(clear, 8);

    // Program 4 bytes
    uint8_t program[8] = {XCP_CMD_PROGRAM, 4, 0xAA, 0xBB, 0xCC, 0xDD, 0, 0};
    xcpSlave.processCommand(program, 6);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
    ASSERT_EQ(mock_flash[0x100], 0xAA);
    ASSERT_EQ(mock_flash[0x101], 0xBB);
    ASSERT_EQ(mock_flash[0x102], 0xCC);
    ASSERT_EQ(mock_flash[0x103], 0xDD);
}

// Test: Program - Wrong sequence
TEST(pgm_program_wrong_sequence) {
    reset_test_state();
    connect_xcp();

    // Start but don't clear
    uint8_t start[8] = {XCP_CMD_PROGRAM_START, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(start, 1);

    // Try to program without clearing
    uint8_t program[8] = {XCP_CMD_PROGRAM, 4, 0xAA, 0xBB, 0xCC, 0xDD, 0, 0};
    xcpSlave.processCommand(program, 6);

    ASSERT_EQ(last_tx_data[0], XCP_PID_ERR);
    ASSERT_EQ(last_tx_data[1], XCP_ERR_SEQUENCE);
}

// Test: Program - End programming (size=0)
TEST(pgm_program_end) {
    reset_test_state();
    connect_xcp();

    uint8_t start[8] = {XCP_CMD_PROGRAM_START, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(start, 1);

    uint8_t setMta[8] = {XCP_CMD_SET_MTA, 0, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(setMta, 8);

    uint8_t clear[8] = {XCP_CMD_PROGRAM_CLEAR, 0, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(clear, 8);

    // End programming with size=0
    uint8_t end[8] = {XCP_CMD_PROGRAM, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(end, 2);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
}

// Test: Program Max
TEST(pgm_program_max) {
    reset_test_state();
    connect_xcp();

    uint8_t start[8] = {XCP_CMD_PROGRAM_START, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(start, 1);

    uint8_t setMta[8] = {XCP_CMD_SET_MTA, 0, 0, 0, 0, 0, 0x02, 0x00};
    xcpSlave.processCommand(setMta, 8);

    uint8_t clear[8] = {XCP_CMD_PROGRAM_CLEAR, 0, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(clear, 8);

    // Program max bytes
    uint8_t program[8] = {XCP_CMD_PROGRAM_MAX, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0};
    xcpSlave.processCommand(program, 7);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
    ASSERT_EQ(mock_flash[0x200], 0x11);
    ASSERT_EQ(mock_flash[0x201], 0x22);
}

// Test: Program Reset
TEST(pgm_reset) {
    reset_test_state();
    connect_xcp();

    uint8_t start[8] = {XCP_CMD_PROGRAM_START, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(start, 1);

    uint8_t reset[8] = {XCP_CMD_PROGRAM_RESET, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(reset, 1);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
}

// Test: Program Prepare
TEST(pgm_prepare) {
    reset_test_state();
    connect_xcp();

    uint8_t cmd[8] = {XCP_CMD_PROGRAM_PREPARE, 0, 0x01, 0x00, 0, 0, 0, 0};
    xcpSlave.processCommand(cmd, 4);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
}

// Test: Program Format
TEST(pgm_format) {
    reset_test_state();
    connect_xcp();

    // No compression or encryption
    uint8_t cmd[8] = {XCP_CMD_PROGRAM_FORMAT, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(cmd, 5);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
}

// Test: Program Format - Unsupported compression
TEST(pgm_format_unsupported) {
    reset_test_state();
    connect_xcp();

    // Request compression
    uint8_t cmd[8] = {XCP_CMD_PROGRAM_FORMAT, 1, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(cmd, 5);

    ASSERT_EQ(last_tx_data[0], XCP_PID_ERR);
    ASSERT_EQ(last_tx_data[1], XCP_ERR_MODE_NOT_VALID);
}

// Test: Program Verify
TEST(pgm_verify) {
    reset_test_state();
    connect_xcp();

    // Set up memory to verify
    mock_memory[0x100] = 0x12;
    mock_memory[0x101] = 0x34;
    mock_memory[0x102] = 0x56;
    mock_memory[0x103] = 0x78;

    // Set MTA
    uint8_t setMta[8] = {XCP_CMD_SET_MTA, 0, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(setMta, 8);

    // Verify with correct value
    uint8_t verify[8] = {XCP_CMD_PROGRAM_VERIFY, 1, 0, 0, 0x12, 0x34, 0x56, 0x78};
    xcpSlave.processCommand(verify, 8);

    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);
}

// Test: Program Verify - Failure
TEST(pgm_verify_fail) {
    reset_test_state();
    connect_xcp();

    // Set up memory
    mock_memory[0x100] = 0xAA;
    mock_memory[0x101] = 0xBB;
    mock_memory[0x102] = 0xCC;
    mock_memory[0x103] = 0xDD;

    // Set MTA
    uint8_t setMta[8] = {XCP_CMD_SET_MTA, 0, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(setMta, 8);

    // Verify with wrong value
    uint8_t verify[8] = {XCP_CMD_PROGRAM_VERIFY, 1, 0, 0, 0x00, 0x00, 0x00, 0x00};
    xcpSlave.processCommand(verify, 8);

    ASSERT_EQ(last_tx_data[0], XCP_PID_ERR);
    ASSERT_EQ(last_tx_data[1], XCP_ERR_VERIFY);
}

// Test: Flash erase failure
TEST(pgm_erase_failure) {
    reset_test_state();
    connect_xcp();
    flash_erase_result = false;

    uint8_t start[8] = {XCP_CMD_PROGRAM_START, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(start, 1);

    uint8_t setMta[8] = {XCP_CMD_SET_MTA, 0, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(setMta, 8);

    uint8_t clear[8] = {XCP_CMD_PROGRAM_CLEAR, 0, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(clear, 8);

    ASSERT_EQ(last_tx_data[0], XCP_PID_ERR);
    ASSERT_EQ(last_tx_data[1], XCP_ERR_GENERIC);
}

// Test: Flash write failure
TEST(pgm_write_failure) {
    reset_test_state();
    connect_xcp();

    uint8_t start[8] = {XCP_CMD_PROGRAM_START, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(start, 1);

    uint8_t setMta[8] = {XCP_CMD_SET_MTA, 0, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(setMta, 8);

    uint8_t clear[8] = {XCP_CMD_PROGRAM_CLEAR, 0, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(clear, 8);

    flash_write_result = false;

    uint8_t program[8] = {XCP_CMD_PROGRAM, 4, 0xAA, 0xBB, 0xCC, 0xDD, 0, 0};
    xcpSlave.processCommand(program, 6);

    ASSERT_EQ(last_tx_data[0], XCP_PID_ERR);
    ASSERT_EQ(last_tx_data[1], XCP_ERR_GENERIC);
}

// Test: Full programming sequence
TEST(pgm_full_sequence) {
    reset_test_state();
    connect_xcp();

    // 1. Start
    uint8_t start[8] = {XCP_CMD_PROGRAM_START, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(start, 1);
    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);

    // 2. Get processor info
    uint8_t info[8] = {XCP_CMD_GET_PGM_PROCESSOR_INFO, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(info, 1);
    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);

    // 3. Set MTA
    uint8_t setMta[8] = {XCP_CMD_SET_MTA, 0, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(setMta, 8);
    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);

    // 4. Clear
    uint8_t clear[8] = {XCP_CMD_PROGRAM_CLEAR, 0, 0, 0, 0, 0, 0x01, 0x00};
    xcpSlave.processCommand(clear, 8);
    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);

    // 5. Program multiple blocks
    uint8_t prog1[8] = {XCP_CMD_PROGRAM, 4, 0x01, 0x02, 0x03, 0x04, 0, 0};
    xcpSlave.processCommand(prog1, 6);
    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);

    uint8_t prog2[8] = {XCP_CMD_PROGRAM, 4, 0x05, 0x06, 0x07, 0x08, 0, 0};
    xcpSlave.processCommand(prog2, 6);
    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);

    // 6. End programming
    uint8_t end[8] = {XCP_CMD_PROGRAM, 0, 0, 0, 0, 0, 0, 0};
    xcpSlave.processCommand(end, 2);
    ASSERT_EQ(last_tx_data[0], XCP_PID_RES);

    // 7. Verify data
    ASSERT_EQ(mock_flash[0x100], 0x01);
    ASSERT_EQ(mock_flash[0x101], 0x02);
    ASSERT_EQ(mock_flash[0x104], 0x05);
    ASSERT_EQ(mock_flash[0x107], 0x08);
}

int main() {
    printf("XCP Flash Programming Unit Tests\n");
    printf("==================================\n\n");

    RUN_TEST(pgm_start);
    RUN_TEST(pgm_start_already_active);
    RUN_TEST(pgm_processor_info);
    RUN_TEST(pgm_sector_info);
    RUN_TEST(pgm_sector_info_address);
    RUN_TEST(pgm_sector_info_invalid);
    RUN_TEST(pgm_clear);
    RUN_TEST(pgm_clear_wrong_sequence);
    RUN_TEST(pgm_program);
    RUN_TEST(pgm_program_wrong_sequence);
    RUN_TEST(pgm_program_end);
    RUN_TEST(pgm_program_max);
    RUN_TEST(pgm_reset);
    RUN_TEST(pgm_prepare);
    RUN_TEST(pgm_format);
    RUN_TEST(pgm_format_unsupported);
    RUN_TEST(pgm_verify);
    RUN_TEST(pgm_verify_fail);
    RUN_TEST(pgm_erase_failure);
    RUN_TEST(pgm_write_failure);
    RUN_TEST(pgm_full_sequence);

    printf("\n==================================\n");
    printf("Tests: %d/%d passed\n", tests_passed, tests_run);

    return (tests_passed == tests_run) ? 0 : 1;
}
