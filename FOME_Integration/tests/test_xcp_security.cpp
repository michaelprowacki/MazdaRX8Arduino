/*
 * XCP Security Unit Tests
 *
 * Tests for XCP seed/key security implementation
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

#define ASSERT_FALSE(x) do { \
    if (x) { \
        printf("FAIL\n    %s:%d: expected false\n", __FILE__, __LINE__); \
        return; \
    } \
} while(0)

static int tests_passed = 0;

// Include security header
#include "../xcp/xcp_security.h"

// Global instance
XcpSecurity xcpSecurity;

// -----------------------------------------
// Tests
// -----------------------------------------

TEST(security_disabled) {
    XcpSecurityConfig config;
    config.enabled = false;
    config.requiredForCal = 0;
    config.requiredForDaq = 0;
    config.requiredForPgm = 0;
    config.secretKey = 0x12345678;

    xcpSecurity.init(config);

    // All resources should be unlocked when security is disabled
    ASSERT_TRUE(xcpSecurity.isUnlocked(XCP_SECURITY_CAL_PAG));
    ASSERT_TRUE(xcpSecurity.isUnlocked(XCP_SECURITY_DAQ));
    ASSERT_TRUE(xcpSecurity.isUnlocked(XCP_SECURITY_PGM));
}

TEST(security_enabled_locked) {
    XcpSecurityConfig config;
    config.enabled = true;
    config.requiredForCal = 1;
    config.requiredForDaq = 1;
    config.requiredForPgm = 1;
    config.secretKey = 0x12345678;

    xcpSecurity.init(config);

    // All resources should be locked initially
    ASSERT_FALSE(xcpSecurity.isUnlocked(XCP_SECURITY_CAL_PAG));
    ASSERT_FALSE(xcpSecurity.isUnlocked(XCP_SECURITY_DAQ));
    ASSERT_FALSE(xcpSecurity.isUnlocked(XCP_SECURITY_PGM));
}

TEST(get_seed_returns_bytes) {
    XcpSecurityConfig config;
    config.enabled = true;
    config.requiredForCal = 1;
    config.requiredForDaq = 0;
    config.requiredForPgm = 0;
    config.secretKey = 0x12345678;

    xcpSecurity.init(config);

    uint8_t seed[XCP_SEED_LENGTH];
    uint8_t seedLen = xcpSecurity.getSeed(XCP_SEED_MODE_0, seed);

    ASSERT_EQ(seedLen, XCP_SEED_LENGTH);
}

TEST(unlock_with_correct_key) {
    XcpSecurityConfig config;
    config.enabled = true;
    config.requiredForCal = 1;
    config.requiredForDaq = 0;
    config.requiredForPgm = 0;
    config.secretKey = 0xDEADBEEF;

    xcpSecurity.init(config);

    // Get seed
    uint8_t seed[XCP_SEED_LENGTH];
    xcpSecurity.getSeed(XCP_SEED_MODE_0, seed);

    // Compute correct key
    uint8_t key[XCP_KEY_LENGTH];
    XcpSecurity::computeKey(seed, XCP_SEED_LENGTH, config.secretKey, key);

    // Unlock should succeed
    ASSERT_TRUE(xcpSecurity.unlock(key, XCP_KEY_LENGTH));
    ASSERT_TRUE(xcpSecurity.isUnlocked(XCP_SECURITY_CAL_PAG));
}

TEST(unlock_with_wrong_key) {
    XcpSecurityConfig config;
    config.enabled = true;
    config.requiredForCal = 1;
    config.requiredForDaq = 0;
    config.requiredForPgm = 0;
    config.secretKey = 0xDEADBEEF;

    xcpSecurity.init(config);

    // Get seed
    uint8_t seed[XCP_SEED_LENGTH];
    xcpSecurity.getSeed(XCP_SEED_MODE_0, seed);

    // Wrong key
    uint8_t wrongKey[XCP_KEY_LENGTH] = {0x00, 0x00, 0x00, 0x00};

    // Unlock should fail
    ASSERT_FALSE(xcpSecurity.unlock(wrongKey, XCP_KEY_LENGTH));
    ASSERT_FALSE(xcpSecurity.isUnlocked(XCP_SECURITY_CAL_PAG));
}

TEST(unlock_wrong_length) {
    XcpSecurityConfig config;
    config.enabled = true;
    config.requiredForCal = 1;
    config.requiredForDaq = 0;
    config.requiredForPgm = 0;
    config.secretKey = 0xDEADBEEF;

    xcpSecurity.init(config);

    uint8_t seed[XCP_SEED_LENGTH];
    xcpSecurity.getSeed(XCP_SEED_MODE_0, seed);

    // Key with wrong length
    uint8_t key[2] = {0x12, 0x34};
    ASSERT_FALSE(xcpSecurity.unlock(key, 2));
}

TEST(already_unlocked_zero_seed) {
    XcpSecurityConfig config;
    config.enabled = true;
    config.requiredForCal = 1;
    config.requiredForDaq = 0;
    config.requiredForPgm = 0;
    config.secretKey = 0xDEADBEEF;

    xcpSecurity.init(config);

    // First unlock
    uint8_t seed[XCP_SEED_LENGTH];
    xcpSecurity.getSeed(XCP_SEED_MODE_0, seed);
    uint8_t key[XCP_KEY_LENGTH];
    XcpSecurity::computeKey(seed, XCP_SEED_LENGTH, config.secretKey, key);
    xcpSecurity.unlock(key, XCP_KEY_LENGTH);

    // Second request should return zero-length seed
    uint8_t seedLen = xcpSecurity.getSeed(XCP_SEED_MODE_0, seed);
    ASSERT_EQ(seedLen, 0);
}

TEST(multiple_resources) {
    XcpSecurityConfig config;
    config.enabled = true;
    config.requiredForCal = 1;
    config.requiredForDaq = 1;
    config.requiredForPgm = 1;
    config.secretKey = 0xCAFEBABE;

    xcpSecurity.init(config);

    // Unlock CAL
    uint8_t seed[XCP_SEED_LENGTH];
    uint8_t key[XCP_KEY_LENGTH];

    xcpSecurity.getSeed(XCP_SEED_MODE_0, seed);
    XcpSecurity::computeKey(seed, XCP_SEED_LENGTH, config.secretKey, key);
    xcpSecurity.unlock(key, XCP_KEY_LENGTH);

    // CAL unlocked, others still locked
    ASSERT_TRUE(xcpSecurity.isUnlocked(XCP_SECURITY_CAL_PAG));
    ASSERT_FALSE(xcpSecurity.isUnlocked(XCP_SECURITY_DAQ));
    ASSERT_FALSE(xcpSecurity.isUnlocked(XCP_SECURITY_PGM));

    // Unlock DAQ
    xcpSecurity.getSeed(XCP_SEED_MODE_1, seed);
    XcpSecurity::computeKey(seed, XCP_SEED_LENGTH, config.secretKey, key);
    xcpSecurity.unlock(key, XCP_KEY_LENGTH);

    ASSERT_TRUE(xcpSecurity.isUnlocked(XCP_SECURITY_CAL_PAG));
    ASSERT_TRUE(xcpSecurity.isUnlocked(XCP_SECURITY_DAQ));
    ASSERT_FALSE(xcpSecurity.isUnlocked(XCP_SECURITY_PGM));
}

TEST(reset_locks_all) {
    XcpSecurityConfig config;
    config.enabled = true;
    config.requiredForCal = 1;
    config.requiredForDaq = 1;
    config.requiredForPgm = 1;
    config.secretKey = 0xDEADBEEF;

    xcpSecurity.init(config);

    // Unlock CAL
    uint8_t seed[XCP_SEED_LENGTH];
    uint8_t key[XCP_KEY_LENGTH];
    xcpSecurity.getSeed(XCP_SEED_MODE_0, seed);
    XcpSecurity::computeKey(seed, XCP_SEED_LENGTH, config.secretKey, key);
    xcpSecurity.unlock(key, XCP_KEY_LENGTH);

    ASSERT_TRUE(xcpSecurity.isUnlocked(XCP_SECURITY_CAL_PAG));

    // Reset
    xcpSecurity.reset();

    // Should be locked again
    ASSERT_FALSE(xcpSecurity.isUnlocked(XCP_SECURITY_CAL_PAG));
}

TEST(pgm_resource_unlock) {
    XcpSecurityConfig config;
    config.enabled = true;
    config.requiredForCal = 0;
    config.requiredForDaq = 0;
    config.requiredForPgm = 1;
    config.secretKey = 0x12345678;

    xcpSecurity.init(config);

    // CAL and DAQ should be unlocked (not required)
    ASSERT_TRUE(xcpSecurity.isUnlocked(XCP_SECURITY_CAL_PAG));
    ASSERT_TRUE(xcpSecurity.isUnlocked(XCP_SECURITY_DAQ));

    // PGM should be locked
    ASSERT_FALSE(xcpSecurity.isUnlocked(XCP_SECURITY_PGM));

    // Unlock PGM
    uint8_t seed[XCP_SEED_LENGTH];
    uint8_t key[XCP_KEY_LENGTH];
    xcpSecurity.getSeed(XCP_SEED_MODE_3, seed);
    XcpSecurity::computeKey(seed, XCP_SEED_LENGTH, config.secretKey, key);
    xcpSecurity.unlock(key, XCP_KEY_LENGTH);

    ASSERT_TRUE(xcpSecurity.isUnlocked(XCP_SECURITY_PGM));
}

TEST(different_seeds_each_request) {
    XcpSecurityConfig config;
    config.enabled = true;
    config.requiredForCal = 1;
    config.requiredForDaq = 0;
    config.requiredForPgm = 0;
    config.secretKey = 0xDEADBEEF;

    xcpSecurity.init(config);

    uint8_t seed1[XCP_SEED_LENGTH];
    uint8_t seed2[XCP_SEED_LENGTH];

    xcpSecurity.getSeed(XCP_SEED_MODE_0, seed1);
    xcpSecurity.reset();  // Reset to get new seed
    xcpSecurity.getSeed(XCP_SEED_MODE_0, seed2);

    // Seeds should be different
    bool different = false;
    for (int i = 0; i < XCP_SEED_LENGTH; i++) {
        if (seed1[i] != seed2[i]) {
            different = true;
            break;
        }
    }
    ASSERT_TRUE(different);
}

TEST(status_transitions) {
    XcpSecurityConfig config;
    config.enabled = true;
    config.requiredForCal = 1;
    config.requiredForDaq = 0;
    config.requiredForPgm = 0;
    config.secretKey = 0xDEADBEEF;

    xcpSecurity.init(config);

    // Initial status should be locked
    ASSERT_EQ(xcpSecurity.getStatus(XCP_SECURITY_CAL_PAG), XCP_SEC_LOCKED);

    // After getting seed
    uint8_t seed[XCP_SEED_LENGTH];
    xcpSecurity.getSeed(XCP_SEED_MODE_0, seed);
    ASSERT_EQ(xcpSecurity.getStatus(XCP_SECURITY_CAL_PAG), XCP_SEC_SEED_REQUESTED);

    // After unlock
    uint8_t key[XCP_KEY_LENGTH];
    XcpSecurity::computeKey(seed, XCP_SEED_LENGTH, config.secretKey, key);
    xcpSecurity.unlock(key, XCP_KEY_LENGTH);
    ASSERT_EQ(xcpSecurity.getStatus(XCP_SECURITY_CAL_PAG), XCP_SEC_UNLOCKED);
}

TEST(key_computation_deterministic) {
    uint8_t seed[4] = {0x12, 0x34, 0x56, 0x78};
    uint32_t secret = 0xDEADBEEF;

    uint8_t key1[4], key2[4];
    XcpSecurity::computeKey(seed, 4, secret, key1);
    XcpSecurity::computeKey(seed, 4, secret, key2);

    // Same inputs should produce same outputs
    for (int i = 0; i < 4; i++) {
        ASSERT_EQ(key1[i], key2[i]);
    }
}

TEST(different_secrets_different_keys) {
    uint8_t seed[4] = {0x12, 0x34, 0x56, 0x78};

    uint8_t key1[4], key2[4];
    XcpSecurity::computeKey(seed, 4, 0xDEADBEEF, key1);
    XcpSecurity::computeKey(seed, 4, 0xCAFEBABE, key2);

    // Different secrets should produce different keys
    bool different = false;
    for (int i = 0; i < 4; i++) {
        if (key1[i] != key2[i]) {
            different = true;
            break;
        }
    }
    ASSERT_TRUE(different);
}

// -----------------------------------------
// Main
// -----------------------------------------

int main() {
    printf("XCP Security Unit Tests\n");
    printf("=======================\n\n");

    printf("Basic Security:\n");
    RUN_TEST(security_disabled);
    RUN_TEST(security_enabled_locked);
    RUN_TEST(get_seed_returns_bytes);

    printf("\nUnlock Operations:\n");
    RUN_TEST(unlock_with_correct_key);
    RUN_TEST(unlock_with_wrong_key);
    RUN_TEST(unlock_wrong_length);
    RUN_TEST(already_unlocked_zero_seed);

    printf("\nMultiple Resources:\n");
    RUN_TEST(multiple_resources);
    RUN_TEST(pgm_resource_unlock);

    printf("\nState Management:\n");
    RUN_TEST(reset_locks_all);
    RUN_TEST(status_transitions);

    printf("\nKey Algorithm:\n");
    RUN_TEST(different_seeds_each_request);
    RUN_TEST(key_computation_deterministic);
    RUN_TEST(different_secrets_different_keys);

    printf("\n=======================\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("=======================\n");

    return 0;
}
