#pragma once

#include "uart.h"

typedef struct {
    serialPort_t *serial;

    int mode;                   // 0 = NMEA, 1 = UBX, 2 = ??
    int present;                // GPS sending some NMEA data?

    int32_t lat;
    int32_t lon;
    float height;               // above mean sea level (m)
    float heading;              // heading
    float speed;
    int fix;
    int numSatellites;
    int validFrames;            // nubmer of received GPS frames

} gpsData_t;

extern gpsData_t gpsData;

enum {
    MODE_NMEA = 0,
    MODE_UBX = 1,
    MODE_MTK = 2,
    MODE_PASSTHROUGH = 3
};

void gpsInit(void);
