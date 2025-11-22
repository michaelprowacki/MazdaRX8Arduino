/*
 * XCP (Universal Measurement and Calibration Protocol) Slave Implementation
 *
 * Implements XCP on CAN for professional calibration tools:
 * - ETAS INCA
 * - Vector CANape
 * - ATI Vision
 */

#include "xcp_protocol.h"
#include <string.h>

// Global instance
XcpSlave xcpSlave;

// Platform-specific CAN transmit (implement in main application)
extern void xcpCanTransmit(uint32_t canId, uint8_t* data, uint8_t length);

// Platform-specific memory access (implement in main application)
extern uint8_t xcpReadByte(uint32_t address);
extern void xcpWriteByte(uint32_t address, uint8_t value);
extern uint32_t xcpGetTimestamp();

void XcpSlave::init(uint32_t rxCanId, uint32_t txCanId) {
    m_rxCanId = rxCanId;
    m_txCanId = txCanId;
    m_connected = false;
    m_mta = 0;
    m_mtaExtension = 0;

    // Clear DAQ lists
    memset(m_daqLists, 0, sizeof(m_daqLists));
}

void XcpSlave::processCommand(uint8_t* data, uint8_t length) {
    if (length < 1) return;

    uint8_t cmd = data[0];

    // Commands that work without connection
    if (cmd == XCP_CMD_CONNECT) {
        cmdConnect(data);
        return;
    }

    // All other commands require connection
    if (!m_connected) {
        sendError(XCP_ERR_SEQUENCE);
        return;
    }

    switch (cmd) {
        case XCP_CMD_DISCONNECT:
            cmdDisconnect();
            break;
        case XCP_CMD_GET_STATUS:
            cmdGetStatus();
            break;
        case XCP_CMD_GET_COMM_MODE_INFO:
            cmdGetCommModeInfo();
            break;
        case XCP_CMD_GET_ID:
            cmdGetId(data);
            break;
        case XCP_CMD_SET_MTA:
            cmdSetMta(data);
            break;
        case XCP_CMD_UPLOAD:
            cmdUpload(data);
            break;
        case XCP_CMD_SHORT_UPLOAD:
            cmdShortUpload(data);
            break;
        case XCP_CMD_DOWNLOAD:
            cmdDownload(data);
            break;
        case XCP_CMD_SHORT_DOWNLOAD:
            cmdShortDownload(data);
            break;

        // DAQ commands
        case XCP_CMD_FREE_DAQ:
            cmdFreeDaq();
            break;
        case XCP_CMD_ALLOC_DAQ:
            cmdAllocDaq(data);
            break;
        case XCP_CMD_ALLOC_ODT:
            cmdAllocOdt(data);
            break;
        case XCP_CMD_ALLOC_ODT_ENTRY:
            cmdAllocOdtEntry(data);
            break;
        case XCP_CMD_SET_DAQ_PTR:
            cmdSetDaqPtr(data);
            break;
        case XCP_CMD_WRITE_DAQ:
            cmdWriteDaq(data);
            break;
        case XCP_CMD_SET_DAQ_LIST_MODE:
            cmdSetDaqListMode(data);
            break;
        case XCP_CMD_START_STOP_DAQ_LIST:
            cmdStartStopDaqList(data);
            break;
        case XCP_CMD_START_STOP_SYNCH:
            cmdStartStopSynch(data);
            break;
        case XCP_CMD_GET_DAQ_PROCESSOR_INFO:
            cmdGetDaqProcessorInfo();
            break;
        case XCP_CMD_GET_DAQ_RESOLUTION_INFO:
            cmdGetDaqResolutionInfo();
            break;
        case XCP_CMD_GET_DAQ_LIST_INFO:
            cmdGetDaqListInfo(data);
            break;
        case XCP_CMD_GET_DAQ_EVENT_INFO:
            cmdGetDaqEventInfo(data);
            break;
        case XCP_CMD_GET_DAQ_CLOCK:
            cmdGetDaqClock();
            break;

        default:
            sendError(XCP_ERR_CMD_UNKNOWN);
            break;
    }
}

void XcpSlave::cmdConnect(uint8_t* data) {
    // data[1] = mode (0 = normal, 1 = user defined)
    m_connected = true;

    uint8_t response[8];
    response[0] = XCP_PID_RES;
    response[1] = XCP_RESOURCE_CAL_PAG | XCP_RESOURCE_DAQ;  // Available resources
    response[2] = 0x00;  // COMM_MODE_BASIC
    response[3] = XCP_MAX_CTO;
    response[4] = (XCP_MAX_DTO >> 8) & 0xFF;
    response[5] = XCP_MAX_DTO & 0xFF;
    response[6] = 0x01;  // XCP protocol version
    response[7] = 0x01;  // Transport layer version

    sendResponse(response, 8);
}

void XcpSlave::cmdDisconnect() {
    m_connected = false;

    // Stop all DAQ lists
    for (int i = 0; i < XCP_MAX_DAQ_LISTS; i++) {
        m_daqLists[i].running = false;
    }

    sendPositiveResponse();
}

void XcpSlave::cmdGetStatus() {
    uint8_t response[6];
    response[0] = XCP_PID_RES;
    response[1] = 0x00;  // Current session status
    response[2] = XCP_RESOURCE_CAL_PAG | XCP_RESOURCE_DAQ;  // Protection status
    response[3] = 0x00;  // Reserved
    response[4] = 0x00;  // Session configuration ID (high)
    response[5] = 0x00;  // Session configuration ID (low)

    sendResponse(response, 6);
}

void XcpSlave::cmdGetCommModeInfo() {
    uint8_t response[8];
    response[0] = XCP_PID_RES;
    response[1] = 0x00;  // Reserved
    response[2] = 0x00;  // COMM_MODE_OPTIONAL
    response[3] = 0x00;  // Reserved
    response[4] = XCP_MAX_CTO - 1;  // MAX_BS (block size)
    response[5] = 0x00;  // MIN_ST (separation time)
    response[6] = 0x01;  // QUEUE_SIZE
    response[7] = 0x01;  // XCP Driver version

    sendResponse(response, 8);
}

void XcpSlave::cmdGetId(uint8_t* data) {
    uint8_t idType = data[1];

    // For now, just return empty ID
    uint8_t response[8];
    response[0] = XCP_PID_RES;
    response[1] = 0x00;  // Mode
    response[2] = 0x00;  // Reserved
    response[3] = 0x00;  // Reserved
    response[4] = 0x00;  // Length (high)
    response[5] = 0x00;  // Length (low)
    response[6] = 0x00;
    response[7] = 0x00;

    sendResponse(response, 8);
}

void XcpSlave::cmdSetMta(uint8_t* data) {
    m_mtaExtension = data[3];
    m_mta = ((uint32_t)data[4] << 24) |
            ((uint32_t)data[5] << 16) |
            ((uint32_t)data[6] << 8) |
            (uint32_t)data[7];

    sendPositiveResponse();
}

void XcpSlave::cmdUpload(uint8_t* data) {
    uint8_t numBytes = data[1];

    if (numBytes > XCP_MAX_CTO - 1) {
        sendError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    uint8_t response[XCP_MAX_CTO];
    response[0] = XCP_PID_RES;

    for (int i = 0; i < numBytes; i++) {
        response[i + 1] = xcpReadByte(m_mta + i);
    }

    m_mta += numBytes;
    sendResponse(response, numBytes + 1);
}

void XcpSlave::cmdShortUpload(uint8_t* data) {
    uint8_t numBytes = data[1];
    uint8_t extension = data[3];
    uint32_t address = ((uint32_t)data[4] << 24) |
                       ((uint32_t)data[5] << 16) |
                       ((uint32_t)data[6] << 8) |
                       (uint32_t)data[7];

    if (numBytes > XCP_MAX_CTO - 1) {
        sendError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    uint8_t response[XCP_MAX_CTO];
    response[0] = XCP_PID_RES;

    for (int i = 0; i < numBytes; i++) {
        response[i + 1] = xcpReadByte(address + i);
    }

    // Update MTA
    m_mta = address + numBytes;
    m_mtaExtension = extension;

    sendResponse(response, numBytes + 1);
}

void XcpSlave::cmdDownload(uint8_t* data) {
    uint8_t numBytes = data[1];

    if (numBytes > XCP_MAX_CTO - 2) {
        sendError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    for (int i = 0; i < numBytes; i++) {
        xcpWriteByte(m_mta + i, data[i + 2]);
    }

    m_mta += numBytes;
    sendPositiveResponse();
}

void XcpSlave::cmdShortDownload(uint8_t* data) {
    uint8_t numBytes = data[1];
    uint8_t extension = data[3];
    uint32_t address = ((uint32_t)data[4] << 24) |
                       ((uint32_t)data[5] << 16) |
                       ((uint32_t)data[6] << 8) |
                       (uint32_t)data[7];

    // For SHORT_DOWNLOAD, data starts at offset 8, but CAN frame is only 8 bytes
    // This command is typically used with CAN-FD or larger packets
    sendError(XCP_ERR_CMD_SYNTAX);
}

// DAQ Command Implementations

void XcpSlave::cmdFreeDaq() {
    // Clear all DAQ lists
    memset(m_daqLists, 0, sizeof(m_daqLists));
    m_currentDaqList = 0;
    m_currentOdt = 0;
    m_currentEntry = 0;

    sendPositiveResponse();
}

void XcpSlave::cmdAllocDaq(uint8_t* data) {
    uint16_t daqCount = ((uint16_t)data[2] << 8) | data[3];

    if (daqCount > XCP_MAX_DAQ_LISTS) {
        sendError(XCP_ERR_MEMORY_OVERFLOW);
        return;
    }

    // Initialize requested number of DAQ lists
    for (int i = 0; i < daqCount; i++) {
        m_daqLists[i].odtCount = 0;
        m_daqLists[i].running = false;
    }

    sendPositiveResponse();
}

void XcpSlave::cmdAllocOdt(uint8_t* data) {
    uint16_t daqListNum = ((uint16_t)data[2] << 8) | data[3];
    uint8_t odtCount = data[4];

    if (daqListNum >= XCP_MAX_DAQ_LISTS) {
        sendError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    if (odtCount > XCP_MAX_ODT_PER_LIST) {
        sendError(XCP_ERR_MEMORY_OVERFLOW);
        return;
    }

    m_daqLists[daqListNum].odtCount = odtCount;

    // Initialize ODTs
    for (int i = 0; i < odtCount; i++) {
        m_daqLists[daqListNum].odts[i].entryCount = 0;
    }

    sendPositiveResponse();
}

void XcpSlave::cmdAllocOdtEntry(uint8_t* data) {
    uint16_t daqListNum = ((uint16_t)data[2] << 8) | data[3];
    uint8_t odtNum = data[4];
    uint8_t entryCount = data[5];

    if (daqListNum >= XCP_MAX_DAQ_LISTS) {
        sendError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    if (odtNum >= m_daqLists[daqListNum].odtCount) {
        sendError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    if (entryCount > XCP_MAX_ENTRIES_PER_ODT) {
        sendError(XCP_ERR_MEMORY_OVERFLOW);
        return;
    }

    m_daqLists[daqListNum].odts[odtNum].entryCount = entryCount;

    sendPositiveResponse();
}

void XcpSlave::cmdSetDaqPtr(uint8_t* data) {
    m_currentDaqList = ((uint16_t)data[2] << 8) | data[3];
    m_currentOdt = data[4];
    m_currentEntry = data[5];

    if (m_currentDaqList >= XCP_MAX_DAQ_LISTS ||
        m_currentOdt >= m_daqLists[m_currentDaqList].odtCount ||
        m_currentEntry >= m_daqLists[m_currentDaqList].odts[m_currentOdt].entryCount) {
        sendError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    sendPositiveResponse();
}

void XcpSlave::cmdWriteDaq(uint8_t* data) {
    uint8_t bitOffset = data[1];
    uint8_t size = data[2];
    uint8_t extension = data[3];
    uint32_t address = ((uint32_t)data[4] << 24) |
                       ((uint32_t)data[5] << 16) |
                       ((uint32_t)data[6] << 8) |
                       (uint32_t)data[7];

    if (m_currentDaqList >= XCP_MAX_DAQ_LISTS) {
        sendError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    XcpOdtEntry* entry = &m_daqLists[m_currentDaqList].odts[m_currentOdt].entries[m_currentEntry];
    entry->address = address;
    entry->size = size;
    entry->extension = extension;

    // Auto-increment entry pointer
    m_currentEntry++;
    if (m_currentEntry >= m_daqLists[m_currentDaqList].odts[m_currentOdt].entryCount) {
        m_currentEntry = 0;
        m_currentOdt++;
        if (m_currentOdt >= m_daqLists[m_currentDaqList].odtCount) {
            m_currentOdt = 0;
        }
    }

    sendPositiveResponse();
}

void XcpSlave::cmdSetDaqListMode(uint8_t* data) {
    uint8_t mode = data[1];
    uint16_t daqListNum = ((uint16_t)data[2] << 8) | data[3];
    uint16_t eventChannel = ((uint16_t)data[4] << 8) | data[5];
    uint8_t prescaler = data[6];
    uint8_t priority = data[7];

    if (daqListNum >= XCP_MAX_DAQ_LISTS) {
        sendError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    m_daqLists[daqListNum].mode = mode;
    m_daqLists[daqListNum].eventChannel = eventChannel;
    m_daqLists[daqListNum].prescaler = prescaler;
    m_daqLists[daqListNum].priority = priority;

    sendPositiveResponse();
}

void XcpSlave::cmdStartStopDaqList(uint8_t* data) {
    uint8_t mode = data[1];
    uint16_t daqListNum = ((uint16_t)data[2] << 8) | data[3];

    if (daqListNum >= XCP_MAX_DAQ_LISTS) {
        sendError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    if (mode == 0) {
        // Stop
        m_daqLists[daqListNum].running = false;
    } else if (mode == 1) {
        // Start
        m_daqLists[daqListNum].running = true;
    } else if (mode == 2) {
        // Select
        // Just acknowledge
    }

    uint8_t response[2];
    response[0] = XCP_PID_RES;
    response[1] = 0x00;  // FIRST_PID

    sendResponse(response, 2);
}

void XcpSlave::cmdStartStopSynch(uint8_t* data) {
    uint8_t mode = data[1];

    if (mode == 0) {
        // Stop all
        for (int i = 0; i < XCP_MAX_DAQ_LISTS; i++) {
            m_daqLists[i].running = false;
        }
    } else if (mode == 1) {
        // Start selected
        for (int i = 0; i < XCP_MAX_DAQ_LISTS; i++) {
            // Start lists that have been configured
            if (m_daqLists[i].odtCount > 0) {
                m_daqLists[i].running = true;
            }
        }
    }

    sendPositiveResponse();
}

void XcpSlave::cmdGetDaqProcessorInfo() {
    uint8_t response[8];
    response[0] = XCP_PID_RES;
    response[1] = 0x01;  // DAQ_PROPERTIES (DAQ supported)
    response[2] = (XCP_MAX_DAQ_LISTS >> 8) & 0xFF;
    response[3] = XCP_MAX_DAQ_LISTS & 0xFF;
    response[4] = (XCP_MAX_DAQ_LISTS >> 8) & 0xFF;  // MAX_EVENT_CHANNEL
    response[5] = XCP_MAX_DAQ_LISTS & 0xFF;
    response[6] = 0x00;  // MIN_DAQ
    response[7] = 0x00;  // DAQ_KEY_BYTE

    sendResponse(response, 8);
}

void XcpSlave::cmdGetDaqResolutionInfo() {
    uint8_t response[8];
    response[0] = XCP_PID_RES;
    response[1] = 0x01;  // GRANULARITY_ODT_ENTRY_SIZE_DAQ
    response[2] = XCP_MAX_ENTRIES_PER_ODT;  // MAX_ODT_ENTRY_SIZE_DAQ
    response[3] = 0x01;  // GRANULARITY_ODT_ENTRY_SIZE_STIM
    response[4] = XCP_MAX_ENTRIES_PER_ODT;  // MAX_ODT_ENTRY_SIZE_STIM
    response[5] = 0x00;  // TIMESTAMP_MODE
    response[6] = 0x00;  // TIMESTAMP_TICKS (high)
    response[7] = 0x01;  // TIMESTAMP_TICKS (low)

    sendResponse(response, 8);
}

void XcpSlave::cmdGetDaqListInfo(uint8_t* data) {
    uint16_t daqListNum = ((uint16_t)data[2] << 8) | data[3];

    if (daqListNum >= XCP_MAX_DAQ_LISTS) {
        sendError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    uint8_t response[6];
    response[0] = XCP_PID_RES;
    response[1] = 0x00;  // DAQ_LIST_PROPERTIES
    response[2] = m_daqLists[daqListNum].odtCount;  // MAX_ODT
    response[3] = XCP_MAX_ENTRIES_PER_ODT;  // MAX_ODT_ENTRIES
    response[4] = 0x00;  // FIXED_EVENT (high)
    response[5] = 0x00;  // FIXED_EVENT (low)

    sendResponse(response, 6);
}

void XcpSlave::cmdGetDaqEventInfo(uint8_t* data) {
    uint16_t eventChannel = ((uint16_t)data[2] << 8) | data[3];

    uint8_t response[7];
    response[0] = XCP_PID_RES;
    response[1] = 0x04;  // DAQ_EVENT_PROPERTIES (DAQ supported)
    response[2] = XCP_MAX_DAQ_LISTS;  // MAX_DAQ_LIST
    response[3] = 0x00;  // EVENT_CHANNEL_NAME_LENGTH
    response[4] = 0x0A;  // EVENT_CHANNEL_TIME_CYCLE (10ms)
    response[5] = 0x06;  // EVENT_CHANNEL_TIME_UNIT (1ms)
    response[6] = 0x00;  // EVENT_CHANNEL_PRIORITY

    sendResponse(response, 7);
}

void XcpSlave::cmdGetDaqClock() {
    uint32_t timestamp = xcpGetTimestamp();

    uint8_t response[8];
    response[0] = XCP_PID_RES;
    response[1] = 0x00;  // Reserved
    response[2] = 0x00;  // Reserved
    response[3] = 0x00;  // Reserved
    response[4] = (timestamp >> 24) & 0xFF;
    response[5] = (timestamp >> 16) & 0xFF;
    response[6] = (timestamp >> 8) & 0xFF;
    response[7] = timestamp & 0xFF;

    sendResponse(response, 8);
}

void XcpSlave::sendDaqData(uint8_t eventChannel) {
    for (int listIdx = 0; listIdx < XCP_MAX_DAQ_LISTS; listIdx++) {
        XcpDaqList* list = &m_daqLists[listIdx];

        if (!list->running || list->eventChannel != eventChannel) {
            continue;
        }

        // Send each ODT
        for (int odtIdx = 0; odtIdx < list->odtCount; odtIdx++) {
            XcpOdt* odt = &list->odts[odtIdx];

            uint8_t packet[XCP_MAX_DTO];
            packet[0] = (listIdx * XCP_MAX_ODT_PER_LIST) + odtIdx;  // PID

            int packetPos = 1;
            for (int entryIdx = 0; entryIdx < odt->entryCount; entryIdx++) {
                XcpOdtEntry* entry = &odt->entries[entryIdx];

                // Read data from memory
                for (int i = 0; i < entry->size && packetPos < XCP_MAX_DTO; i++) {
                    packet[packetPos++] = xcpReadByte(entry->address + i);
                }
            }

            canTransmit(packet, packetPos);
        }
    }
}

// Response Helpers

void XcpSlave::sendResponse(uint8_t* data, uint8_t length) {
    canTransmit(data, length);
}

void XcpSlave::sendError(uint8_t errorCode) {
    uint8_t response[2];
    response[0] = XCP_PID_ERR;
    response[1] = errorCode;
    canTransmit(response, 2);
}

void XcpSlave::sendPositiveResponse() {
    uint8_t response[1];
    response[0] = XCP_PID_RES;
    canTransmit(response, 1);
}

void XcpSlave::canTransmit(uint8_t* data, uint8_t length) {
    xcpCanTransmit(m_txCanId, data, length);
}
