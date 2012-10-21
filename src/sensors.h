#pragma once

typedef struct {
    uint8_t numCells;
    float voltage[6];
} batteryData_t;

typedef struct {
    int start;

    uint8_t numPktBytes;
    uint8_t dataState;
    uint8_t frskyRxBuffer[19];  // Receive buffer. 9 bytes (full packet), worst case 18 bytes with byte-stuffing (+1)
    uint8_t FrskyRxBufferReady;

    batteryData_t batteryData;
    uint16_t amps;
    uint16_t volts_bp;          // 0x3A
    uint16_t volts_ap;          // 0x3B
    float volts;

} sensorData_t;

void sensorsInit(void);

extern sensorData_t sensorData;

void processFrskyPacket(void);
