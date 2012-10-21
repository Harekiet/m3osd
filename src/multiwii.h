#pragma once

#include "uart.h"

#define MSP_BUFFER_SIZE 128

typedef struct {
    serialPort_t *serial;

    uint8_t mspParseOK;
    uint8_t buffer[MSP_BUFFER_SIZE];
    uint8_t cmdMSP;
    uint8_t dataSize;
    uint8_t rxIndex;
    uint8_t rdIndex;

    // IMU data
    int angleRoll;
    int anglePitch;
    int heading;
    int altitude;

    // IMU GPS data
    uint8_t GPS_FIX;
    uint8_t GPS_numSat;
    int32_t GPS_LAT;
    int32_t GPS_LON;
    int16_t GPS_altitude;       // altitude in 0.1m
    int16_t GPS_speed;          // speed in 0.1m/s
    uint8_t GPS_update;         // New data signalisation to GPS functions
    uint16_t GPS_distanceToHome;
    uint16_t GPS_directionToHome;
    int32_t GPS_homeLAT;
    int32_t GPS_homeLON;

} multiwiiData_t;

extern multiwiiData_t multiwiiData;

// Multiwii Serial Protocol 0 
#define MSP_VERSION              0
#define PLATFORM_32BIT           0x80000000

#define MSP_IDENT                100    //out message         multitype + version
#define MSP_STATUS               101    //out message         cycletime & errors_count & sensor present & box activation
#define MSP_RAW_IMU              102    //out message         9 DOF
#define MSP_SERVO                103    //out message         8 servos
#define MSP_MOTOR                104    //out message         8 motors
#define MSP_RC                   105    //out message         8 rc chan
#define MSP_RAW_GPS              106    //out message         fix, numsat, lat, lon, alt, speed
#define MSP_COMP_GPS             107    //out message         distance home, direction home
#define MSP_ATTITUDE             108    //out message         2 angles 1 heading
#define MSP_ALTITUDE             109    //out message         1 altitude
#define MSP_BAT                  110    //out message         vbat, powermetersum
#define MSP_RC_TUNING            111    //out message         rc rate, rc expo, rollpitch rate, yaw rate, dyn throttle PID
#define MSP_PID                  112    //out message         up to 16 P I D (8 are used)
#define MSP_BOX                  113    //out message         up to 16 checkbox (11 are used)
#define MSP_MISC                 114    //out message         powermeter trig + 8 free for future use
#define MSP_MOTOR_PINS           115    //out message         which pins are in use for motors & servos, for GUI
#define MSP_BOXNAMES             116    //out message         the aux switch names
#define MSP_PIDNAMES             117    //out message         the PID names
#define MSP_WP                   118    //out message         get a WP, WP# is in the payload, returns (WP#, lat, lon, alt, flags) WP#0-home, WP#16-poshold

#define MSP_SET_RAW_RC           200    //in message          8 rc chan
#define MSP_SET_RAW_GPS          201    //in message          fix, numsat, lat, lon, alt, speed
#define MSP_SET_PID              202    //in message          up to 16 P I D (8 are used)
#define MSP_SET_BOX              203    //in message          up to 16 checkbox (11 are used)
#define MSP_SET_RC_TUNING        204    //in message          rc rate, rc expo, rollpitch rate, yaw rate, dyn throttle PID
#define MSP_ACC_CALIBRATION      205    //in message          no param
#define MSP_MAG_CALIBRATION      206    //in message          no param
#define MSP_SET_MISC             207    //in message          powermeter trig + 8 free for future use
#define MSP_RESET_CONF           208    //in message          no param
#define MSP_WP_SET               209    //in message          sets a given WP (WP#,lat, lon, alt, flags)

#define MSP_EEPROM_WRITE         250    //in message          no param

#define MSP_DEBUG                254    //out message         debug1,debug2,debug3,debug4

void multiwiiInit(void);
