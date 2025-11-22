/*
 * XCP (Universal Measurement and Calibration Protocol) Slave
 *
 * Enables professional calibration tools:
 * - ETAS INCA
 * - Vector CANape
 * - ATI Vision
 *
 * Implements XCP on CAN transport layer
 */

#ifndef XCP_PROTOCOL_H
#define XCP_PROTOCOL_H

#include <stdint.h>

// XCP Command Codes (Standard)
#define XCP_CMD_CONNECT             0xFF
#define XCP_CMD_DISCONNECT          0xFE
#define XCP_CMD_GET_STATUS          0xFD
#define XCP_CMD_SYNCH               0xFC
#define XCP_CMD_GET_COMM_MODE_INFO  0xFB
#define XCP_CMD_GET_ID              0xFA
#define XCP_CMD_SET_REQUEST         0xF9
#define XCP_CMD_GET_SEED            0xF8
#define XCP_CMD_UNLOCK              0xF7
#define XCP_CMD_SET_MTA             0xF6
#define XCP_CMD_UPLOAD              0xF5
#define XCP_CMD_SHORT_UPLOAD        0xF4
#define XCP_CMD_BUILD_CHECKSUM      0xF3
#define XCP_CMD_DOWNLOAD            0xF0
#define XCP_CMD_DOWNLOAD_NEXT       0xEF
#define XCP_CMD_DOWNLOAD_MAX        0xEE
#define XCP_CMD_SHORT_DOWNLOAD      0xED
#define XCP_CMD_MODIFY_BITS         0xEC

// DAQ Commands
#define XCP_CMD_SET_DAQ_PTR         0xE2
#define XCP_CMD_WRITE_DAQ           0xE1
#define XCP_CMD_SET_DAQ_LIST_MODE   0xE0
#define XCP_CMD_GET_DAQ_LIST_MODE   0xDF
#define XCP_CMD_START_STOP_DAQ_LIST 0xDE
#define XCP_CMD_START_STOP_SYNCH    0xDD
#define XCP_CMD_GET_DAQ_CLOCK       0xDC
#define XCP_CMD_READ_DAQ            0xDB
#define XCP_CMD_GET_DAQ_PROCESSOR_INFO 0xDA
#define XCP_CMD_GET_DAQ_RESOLUTION_INFO 0xD9
#define XCP_CMD_GET_DAQ_LIST_INFO   0xD8
#define XCP_CMD_GET_DAQ_EVENT_INFO  0xD7
#define XCP_CMD_FREE_DAQ            0xD6
#define XCP_CMD_ALLOC_DAQ           0xD5
#define XCP_CMD_ALLOC_ODT           0xD4
#define XCP_CMD_ALLOC_ODT_ENTRY     0xD3

// Response Codes
#define XCP_PID_RES                 0xFF  // Positive response
#define XCP_PID_ERR                 0xFE  // Error
#define XCP_PID_EV                  0xFD  // Event
#define XCP_PID_SERV                0xFC  // Service request

// Error Codes
#define XCP_ERR_CMD_SYNCH           0x00
#define XCP_ERR_CMD_BUSY            0x10
#define XCP_ERR_DAQ_ACTIVE          0x11
#define XCP_ERR_PGM_ACTIVE          0x12
#define XCP_ERR_CMD_UNKNOWN         0x20
#define XCP_ERR_CMD_SYNTAX          0x21
#define XCP_ERR_OUT_OF_RANGE        0x22
#define XCP_ERR_WRITE_PROTECTED     0x23
#define XCP_ERR_ACCESS_DENIED       0x24
#define XCP_ERR_ACCESS_LOCKED       0x25
#define XCP_ERR_PAGE_NOT_VALID      0x26
#define XCP_ERR_MODE_NOT_VALID      0x27
#define XCP_ERR_SEGMENT_NOT_VALID   0x28
#define XCP_ERR_SEQUENCE            0x29
#define XCP_ERR_DAQ_CONFIG          0x2A
#define XCP_ERR_MEMORY_OVERFLOW     0x30
#define XCP_ERR_GENERIC             0x31
#define XCP_ERR_VERIFY              0x32

// Resources
#define XCP_RESOURCE_CAL_PAG        0x01
#define XCP_RESOURCE_DAQ            0x04
#define XCP_RESOURCE_STIM           0x08
#define XCP_RESOURCE_PGM            0x10

// Configuration
#define XCP_MAX_CTO                 8     // Max command packet size
#define XCP_MAX_DTO                 8     // Max data packet size
#define XCP_MAX_DAQ_LISTS           4     // Number of DAQ lists
#define XCP_MAX_ODT_PER_LIST        8     // ODTs per DAQ list
#define XCP_MAX_ENTRIES_PER_ODT     7     // Entries per ODT

// DAQ structures
struct XcpOdtEntry {
    uint32_t address;
    uint8_t size;
    uint8_t extension;
};

struct XcpOdt {
    XcpOdtEntry entries[XCP_MAX_ENTRIES_PER_ODT];
    uint8_t entryCount;
};

struct XcpDaqList {
    XcpOdt odts[XCP_MAX_ODT_PER_LIST];
    uint8_t odtCount;
    uint8_t mode;
    uint16_t eventChannel;
    uint8_t prescaler;
    uint8_t priority;
    bool running;
};

class XcpSlave {
public:
    // Initialize XCP slave
    void init(uint32_t rxCanId, uint32_t txCanId);

    // Process incoming XCP command
    void processCommand(uint8_t* data, uint8_t length);

    // Send DAQ data (call periodically based on event channel)
    void sendDaqData(uint8_t eventChannel);

    // Get connection status
    bool isConnected() { return m_connected; }

private:
    // CAN IDs
    uint32_t m_rxCanId;
    uint32_t m_txCanId;

    // Connection state
    bool m_connected = false;

    // Memory Transfer Address
    uint32_t m_mta = 0;
    uint8_t m_mtaExtension = 0;

    // DAQ configuration
    XcpDaqList m_daqLists[XCP_MAX_DAQ_LISTS];
    uint8_t m_currentDaqList = 0;
    uint8_t m_currentOdt = 0;
    uint8_t m_currentEntry = 0;

    // Command handlers
    void cmdConnect(uint8_t* data);
    void cmdDisconnect();
    void cmdGetStatus();
    void cmdGetCommModeInfo();
    void cmdGetId(uint8_t* data);
    void cmdSetMta(uint8_t* data);
    void cmdUpload(uint8_t* data);
    void cmdShortUpload(uint8_t* data);
    void cmdDownload(uint8_t* data);
    void cmdShortDownload(uint8_t* data);

    // DAQ command handlers
    void cmdFreeDaq();
    void cmdAllocDaq(uint8_t* data);
    void cmdAllocOdt(uint8_t* data);
    void cmdAllocOdtEntry(uint8_t* data);
    void cmdSetDaqPtr(uint8_t* data);
    void cmdWriteDaq(uint8_t* data);
    void cmdSetDaqListMode(uint8_t* data);
    void cmdStartStopDaqList(uint8_t* data);
    void cmdStartStopSynch(uint8_t* data);
    void cmdGetDaqProcessorInfo();
    void cmdGetDaqResolutionInfo();
    void cmdGetDaqListInfo(uint8_t* data);
    void cmdGetDaqEventInfo(uint8_t* data);
    void cmdGetDaqClock();

    // Response helpers
    void sendResponse(uint8_t* data, uint8_t length);
    void sendError(uint8_t errorCode);
    void sendPositiveResponse();

    // CAN transmit (implement in platform layer)
    void canTransmit(uint8_t* data, uint8_t length);
};

// Global XCP slave instance
extern XcpSlave xcpSlave;

#endif // XCP_PROTOCOL_H
