/*
 * XCP Seed/Key Security
 *
 * Implements XCP security access (GET_SEED/UNLOCK) for protected resources
 *
 * Security levels:
 * - Level 0: Public access (no unlock required)
 * - Level 1: Calibration (CAL/PAG resource)
 * - Level 2: Programming (PGM resource)
 * - Level 3: DAQ/STIM (measurement resources)
 */

#ifndef XCP_SECURITY_H
#define XCP_SECURITY_H

#include <stdint.h>

// Security resource masks
#define XCP_SECURITY_CAL_PAG    0x01
#define XCP_SECURITY_DAQ        0x04
#define XCP_SECURITY_STIM       0x08
#define XCP_SECURITY_PGM        0x10

// Security modes (per ASAM XCP spec)
#define XCP_SEED_MODE_0         0x00  // Request seed for CAL/PAG
#define XCP_SEED_MODE_1         0x01  // Request seed for DAQ
#define XCP_SEED_MODE_2         0x02  // Request seed for STIM
#define XCP_SEED_MODE_3         0x03  // Request seed for PGM

// Seed length
#define XCP_SEED_LENGTH         4
#define XCP_KEY_LENGTH          4

// Security status
enum XcpSecurityStatus {
    XCP_SEC_LOCKED = 0,
    XCP_SEC_SEED_REQUESTED,
    XCP_SEC_UNLOCKED
};

// Security configuration
struct XcpSecurityConfig {
    bool enabled;                      // Security enabled
    uint8_t requiredForCal;           // Require unlock for calibration
    uint8_t requiredForDaq;           // Require unlock for DAQ
    uint8_t requiredForPgm;           // Require unlock for programming
    uint32_t secretKey;               // Master key for algorithm
};

class XcpSecurity {
public:
    // Initialize security module
    void init(const XcpSecurityConfig& config);

    // Reset all unlocks (e.g., on disconnect)
    void reset();

    // Check if resource is unlocked
    bool isUnlocked(uint8_t resource);

    // Request seed for resource
    // Returns seed length, fills seed buffer
    uint8_t getSeed(uint8_t mode, uint8_t* seed);

    // Attempt to unlock with key
    // Returns true if unlock successful
    bool unlock(uint8_t* key, uint8_t keyLength);

    // Get current security status
    XcpSecurityStatus getStatus(uint8_t resource);

    // Compute key from seed (for external key calculator)
    static void computeKey(const uint8_t* seed, uint8_t seedLength,
                          uint32_t secretKey, uint8_t* key);

private:
    XcpSecurityConfig m_config;

    // Current state
    uint8_t m_currentMode;
    uint8_t m_currentSeed[XCP_SEED_LENGTH];
    XcpSecurityStatus m_status[4];  // One per mode

    // Unlock state per resource
    bool m_calUnlocked;
    bool m_daqUnlocked;
    bool m_stimUnlocked;
    bool m_pgmUnlocked;

    // Generate random seed
    void generateSeed(uint8_t* seed);

    // Verify key against seed
    bool verifyKey(const uint8_t* key, uint8_t keyLength);
};

// -----------------------------------------
// Implementation
// -----------------------------------------

inline void XcpSecurity::init(const XcpSecurityConfig& config) {
    m_config = config;
    reset();
}

inline void XcpSecurity::reset() {
    m_currentMode = 0;
    m_calUnlocked = false;
    m_daqUnlocked = false;
    m_stimUnlocked = false;
    m_pgmUnlocked = false;

    for (int i = 0; i < 4; i++) {
        m_status[i] = XCP_SEC_LOCKED;
    }

    for (int i = 0; i < XCP_SEED_LENGTH; i++) {
        m_currentSeed[i] = 0;
    }
}

inline bool XcpSecurity::isUnlocked(uint8_t resource) {
    if (!m_config.enabled) {
        return true;  // Security disabled, always unlocked
    }

    if (resource & XCP_SECURITY_CAL_PAG) {
        if (m_config.requiredForCal && !m_calUnlocked) return false;
    }
    if (resource & XCP_SECURITY_DAQ) {
        if (m_config.requiredForDaq && !m_daqUnlocked) return false;
    }
    if (resource & XCP_SECURITY_STIM) {
        if (m_config.requiredForDaq && !m_stimUnlocked) return false;
    }
    if (resource & XCP_SECURITY_PGM) {
        if (m_config.requiredForPgm && !m_pgmUnlocked) return false;
    }

    return true;
}

inline uint8_t XcpSecurity::getSeed(uint8_t mode, uint8_t* seed) {
    m_currentMode = mode;

    // Check if already unlocked
    bool* unlocked = nullptr;
    switch (mode) {
        case XCP_SEED_MODE_0: unlocked = &m_calUnlocked; break;
        case XCP_SEED_MODE_1: unlocked = &m_daqUnlocked; break;
        case XCP_SEED_MODE_2: unlocked = &m_stimUnlocked; break;
        case XCP_SEED_MODE_3: unlocked = &m_pgmUnlocked; break;
    }

    if (unlocked && *unlocked) {
        // Already unlocked, return zero-length seed
        return 0;
    }

    // Generate new seed
    generateSeed(m_currentSeed);

    // Copy to output
    for (int i = 0; i < XCP_SEED_LENGTH; i++) {
        seed[i] = m_currentSeed[i];
    }

    if (mode < 4) {
        m_status[mode] = XCP_SEC_SEED_REQUESTED;
    }

    return XCP_SEED_LENGTH;
}

inline bool XcpSecurity::unlock(uint8_t* key, uint8_t keyLength) {
    if (!verifyKey(key, keyLength)) {
        return false;
    }

    // Unlock the requested resource
    switch (m_currentMode) {
        case XCP_SEED_MODE_0:
            m_calUnlocked = true;
            m_status[0] = XCP_SEC_UNLOCKED;
            break;
        case XCP_SEED_MODE_1:
            m_daqUnlocked = true;
            m_status[1] = XCP_SEC_UNLOCKED;
            break;
        case XCP_SEED_MODE_2:
            m_stimUnlocked = true;
            m_status[2] = XCP_SEC_UNLOCKED;
            break;
        case XCP_SEED_MODE_3:
            m_pgmUnlocked = true;
            m_status[3] = XCP_SEC_UNLOCKED;
            break;
    }

    return true;
}

inline XcpSecurityStatus XcpSecurity::getStatus(uint8_t resource) {
    if (!m_config.enabled) {
        return XCP_SEC_UNLOCKED;
    }

    // Map resource to mode
    uint8_t mode = 0;
    if (resource & XCP_SECURITY_DAQ) mode = 1;
    if (resource & XCP_SECURITY_STIM) mode = 2;
    if (resource & XCP_SECURITY_PGM) mode = 3;

    if (mode < 4) {
        return m_status[mode];
    }
    return XCP_SEC_LOCKED;
}

inline void XcpSecurity::generateSeed(uint8_t* seed) {
    // Simple LFSR-based pseudo-random generator
    // In production, use hardware RNG if available
    static uint32_t lfsr = 0xACE1u;

    for (int i = 0; i < XCP_SEED_LENGTH; i++) {
        uint32_t bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1;
        lfsr = (lfsr >> 1) | (bit << 15);
        seed[i] = lfsr & 0xFF;

        // Mix in secret key
        seed[i] ^= (m_config.secretKey >> (i * 8)) & 0xFF;
    }
}

inline bool XcpSecurity::verifyKey(const uint8_t* key, uint8_t keyLength) {
    if (keyLength != XCP_KEY_LENGTH) {
        return false;
    }

    // Compute expected key
    uint8_t expectedKey[XCP_KEY_LENGTH];
    computeKey(m_currentSeed, XCP_SEED_LENGTH, m_config.secretKey, expectedKey);

    // Compare
    for (int i = 0; i < XCP_KEY_LENGTH; i++) {
        if (key[i] != expectedKey[i]) {
            return false;
        }
    }

    return true;
}

// Key computation algorithm
// This is a simple example - replace with your own algorithm
inline void XcpSecurity::computeKey(const uint8_t* seed, uint8_t seedLength,
                                    uint32_t secretKey, uint8_t* key) {
    // XOR-based key derivation (example only!)
    // Real implementations should use proper cryptographic algorithms
    uint32_t temp = 0;

    // Combine seed with secret
    for (int i = 0; i < seedLength; i++) {
        temp ^= ((uint32_t)seed[i]) << ((i % 4) * 8);
    }
    temp ^= secretKey;

    // Rotate and mix
    temp = ((temp << 13) | (temp >> 19)) ^ 0xDEADBEEF;
    temp = ((temp << 7) | (temp >> 25)) ^ secretKey;

    // Extract key bytes
    key[0] = (temp >> 24) & 0xFF;
    key[1] = (temp >> 16) & 0xFF;
    key[2] = (temp >> 8) & 0xFF;
    key[3] = temp & 0xFF;
}

// Global security instance
extern XcpSecurity xcpSecurity;

#endif // XCP_SECURITY_H
