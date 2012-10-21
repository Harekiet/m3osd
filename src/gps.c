#include "board.h"
#include "uart.h"
#include "gps.h"

#define GPS_STACK_SIZE 128
OS_STK gpsStack[GPS_STACK_SIZE];
gpsData_t gpsData;

static bool gpsParseUBLOX(void);

static void confGPS(const uint8_t * s, uint8_t size)
{
    int i;

    for (i = 0; i < size; i++) {
        uartWrite(gpsData.serial, s[i]);
        timerDelay(26);         // simulating a 38400baud pace (or less), otherwise commands are not accepted by the device.
    }
}

///////////////////////////////////////////////////////////////////////////////
// String to Float Conversion
///////////////////////////////////////////////////////////////////////////////

// Simple and fast atof (ascii to float) function.
//
// - Executes about 5x faster than standard MSCRT library atof().
// - An attractive alternative if the number of calls is in the millions.
// - Assumes input is a proper integer, fraction, or scientific format.
// - Matches library atof() to 15 digits (except at extreme exponents).
// - Follows atof() precedent of essentially no error checking.
//
// 09-May-2009 Tom Van Baak (tvb) www.LeapSecond.com
//

#define white_space(c) ((c) == ' ' || (c) == '\t')
#define valid_digit(c) ((c) >= '0' && (c) <= '9')

float stringToFloat(const char *p)
{
    int frac = 0;
    double sign, value, scale;

    // Skip leading white space, if any.

    while (white_space(*p)) {
        p += 1;
    }

    // Get sign, if any.

    sign = 1.0;
    if (*p == '-') {
        sign = -1.0;
        p += 1;

    } else if (*p == '+') {
        p += 1;
    }
    // Get digits before decimal point or exponent, if any.

    value = 0.0;
    while (valid_digit(*p)) {
        value = value * 10.0 + (*p - '0');
        p += 1;
    }

    // Get digits after decimal point, if any.

    if (*p == '.') {
        double pow10 = 10.0;
        p += 1;

        while (valid_digit(*p)) {
            value += (*p - '0') / pow10;
            pow10 *= 10.0;
            p += 1;
        }
    }
    // Handle exponent, if any.

    scale = 1.0;
    if ((*p == 'e') || (*p == 'E')) {
        unsigned int expon;
        p += 1;

        // Get sign of exponent, if any.

        frac = 0;
        if (*p == '-') {
            frac = 1;
            p += 1;

        } else if (*p == '+') {
            p += 1;
        }
        // Get digits of exponent, if any.

        expon = 0;
        while (valid_digit(*p)) {
            expon = expon * 10 + (*p - '0');
            p += 1;
        }
        if (expon > 308)
            expon = 308;

        // Calculate scaling factor.

        while (expon >= 50) {
            scale *= 1E50;
            expon -= 50;
        }
        while (expon >= 8) {
            scale *= 1E8;
            expon -= 8;
        }
        while (expon > 0) {
            scale *= 10.0;
            expon -= 1;
        }
    }
    // Return signed and scaled floating point result.

    return sign * (frac ? (value / scale) : (value * scale));
}

#define DIGIT_TO_VAL(_x)    (_x - '0')

uint32_t GPS_coord_to_degrees(char *s)
{
    char *p, *q;
    uint8_t deg = 0, min = 0;
    unsigned int frac_min = 0;
    int i;

    // scan for decimal point or end of field
    for (p = s; isdigit(*p); p++);
    q = s;

    // convert degrees
    while ((p - q) > 2) {
        if (deg)
            deg *= 10;
        deg += DIGIT_TO_VAL(*q++);
    }
    // convert minutes
    while (p > q) {
        if (min)
            min *= 10;
        min += DIGIT_TO_VAL(*q++);
    }
    // convert fractional minutes
    // expect up to four digits, result is in
    // ten-thousandths of a minute
    if (*p == '.') {
        q = p + 1;
        for (i = 0; i < 4; i++) {
            frac_min *= 10;
            if (isdigit(*q))
                frac_min += *q++ - '0';
        }
    }
    return deg * 10000000UL + (min * 1000000UL + frac_min * 100UL) / 6;
}

static uint32_t grab_fields(char *src, uint8_t mult)
{
    // convert string to uint32
    uint32_t i;
    uint32_t tmp = 0;
    for (i = 0; src[i] != 0; i++) {
        if (src[i] == '.') {
            i++;
            if (mult == 0)
                break;
            else
                src[i + mult] = 0;
        }
        tmp *= 10;
        if (src[i] >= '0' && src[i] <= '9')
            tmp += src[i] - '0';
    }
    return tmp;
}

static uint8_t hex_c(uint8_t n)
{                               // convert '0'..'9','A'..'F' to 0..15
    n -= '0';
    if (n > 9)
        n -= 7;
    n &= 0x0F;
    return n;
}

/* This is a light implementation of a GPS frame decoding
   This should work with most of modern GPS devices configured to output NMEA frames.
   It assumes there are some NMEA GGA frames to decode on the serial bus
   Here we use only the following data :
     - latitude
     - longitude
     - GPS fix is/is not ok
     - GPS num sat (4 is enough to be +/- reliable)
     // added by Mis
     - GPS altitude (for OSD displaying)
     - GPS speed (for OSD displaying)
*/
#define FRAME_GGA  1
#define FRAME_RMC  2

static int gpsNewFrameNMEA(char c)
{
    uint8_t frameOK = 0;
    static uint8_t param = 0, offset = 0, parity = 0;
    static char string[15];
    static uint8_t checksum_param, frame = 0;

    if (c == '$') {
        param = 0;
        offset = 0;
        parity = 0;
    } else if (c == ',' || c == '*') {
        string[offset] = 0;
        if (param == 0) {       // frame identification
            frame = 0;
            if (string[0] == 'G' && string[1] == 'P' && string[2] == 'G' && string[3] == 'G' && string[4] == 'A')
                frame = FRAME_GGA;
            if (string[0] == 'G' && string[1] == 'P' && string[2] == 'R' && string[3] == 'M' && string[4] == 'C')
                frame = FRAME_RMC;
        } else if (frame == FRAME_GGA) {
            if (param == 2) {
                gpsData.lat = GPS_coord_to_degrees(string);
            } else if (param == 3 && string[0] == 'S')
                gpsData.lat = -gpsData.lat;
            else if (param == 4) {
                gpsData.lon = GPS_coord_to_degrees(string);
            } else if (param == 5 && string[0] == 'W')
                gpsData.lon = -gpsData.lon;
            else if (param == 6) {
                gpsData.fix = string[0] > '0';
            } else if (param == 7) {
                gpsData.numSatellites = grab_fields(string, 0);
            } else if (param == 9) {
                gpsData.height = stringToFloat(string);
            }
        } else if (frame == FRAME_RMC) {
            if (param == 7) {
                gpsData.speed = stringToFloat(string) * 51.44444f;      // speed in cm/s
            }
        }
        param++;
        offset = 0;
        if (c == '*')
            checksum_param = 1;
        else
            parity ^= c;
    } else if (c == '\r' || c == '\n') {
        if (checksum_param) {   // parity checksum
            uint8_t checksum = hex_c(string[0]);
            checksum <<= 4;
            checksum += hex_c(string[1]);
            if (checksum == parity)
                frameOK = 1;
        }
        checksum_param = 0;
    } else {
        if (offset < 15)
            string[offset++] = c;
        if (!checksum_param)
            parity ^= c;
    }
    if (frame)
        gpsData.present = 1;
    return frameOK && (frame == FRAME_GGA);
}

typedef struct {
    uint8_t preamble1;
    uint8_t preamble2;
    uint8_t msg_class;
    uint8_t msg_id;
    uint16_t length;
} ubx_header;

typedef struct {
    uint32_t time;              // GPS msToW
    int32_t longitude;
    int32_t latitude;
    int32_t altitude_ellipsoid;
    int32_t altitude_msl;
    uint32_t horizontal_accuracy;
    uint32_t vertical_accuracy;
} ubx_nav_posllh;

typedef struct {
    uint32_t time;              // GPS msToW
    uint8_t fix_type;
    uint8_t fix_status;
    uint8_t differential_status;
    uint8_t res;
    uint32_t time_to_first_fix;
    uint32_t uptime;            // milliseconds
} ubx_nav_status;

typedef struct {
    uint32_t time;
    int32_t time_nsec;
    int16_t week;
    uint8_t fix_type;
    uint8_t fix_status;
    int32_t ecef_x;
    int32_t ecef_y;
    int32_t ecef_z;
    uint32_t position_accuracy_3d;
    int32_t ecef_x_velocity;
    int32_t ecef_y_velocity;
    int32_t ecef_z_velocity;
    uint32_t speed_accuracy;
    uint16_t position_DOP;
    uint8_t res;
    uint8_t satellites;
    uint32_t res2;
} ubx_nav_solution;

typedef struct {
    uint32_t time;              // GPS msToW
    int32_t ned_north;
    int32_t ned_east;
    int32_t ned_down;
    uint32_t speed_3d;
    uint32_t speed_2d;
    int32_t heading_2d;
    uint32_t speed_accuracy;
    uint32_t heading_accuracy;
} ubx_nav_velned;

enum ubs_protocol_bytes {
    PREAMBLE1 = 0xb5,
    PREAMBLE2 = 0x62,
    CLASS_NAV = 0x01,
    CLASS_ACK = 0x05,
    CLASS_CFG = 0x06,
    MSG_ACK_NACK = 0x00,
    MSG_ACK_ACK = 0x01,
    MSG_POSLLH = 0x2,
    MSG_STATUS = 0x3,
    MSG_SOL = 0x6,
    MSG_VELNED = 0x12,
    MSG_CFG_PRT = 0x00,
    MSG_CFG_RATE = 0x08,
    MSG_CFG_SET_RATE = 0x01,
    MSG_CFG_NAV_SETTINGS = 0x24
};
enum ubs_nav_fix_type {
    FIX_NONE = 0,
    FIX_DEAD_RECKONING = 1,
    FIX_2D = 2,
    FIX_3D = 3,
    FIX_GPS_DEAD_RECKONING = 4,
    FIX_TIME = 5
};
enum ubx_nav_status_bits {
    NAV_STATUS_FIX_VALID = 1
};

// Packet checksum accumulators
static uint8_t _ck_a;
static uint8_t _ck_b;

// State machine state
static uint8_t _step;
static uint8_t _msg_id;
static uint16_t _payload_length;
static uint16_t _payload_counter;

static bool next_fix;
static uint8_t _class;

// do we have new position information?
static bool _new_position;

// do we have new speed information?
static bool _new_speed;

static uint8_t _disable_counter;

// Receive buffer
static union {
    ubx_nav_posllh posllh;
    ubx_nav_status status;
    ubx_nav_solution solution;
    ubx_nav_velned velned;
    uint8_t bytes[64];
} _buffer;

void _update_checksum(uint8_t * data, uint8_t len, uint8_t * ck_a, uint8_t * ck_b)
{
    while (len--) {
        *ck_a += *data;
        *ck_b += *ck_a;
        data++;
    }
}

bool gpsNewFrameUBLOX(uint8_t data)
{
    bool parsed = false;

    switch (_step) {
    case 1:
        if (PREAMBLE2 == data) {
            _step++;
            break;
        }
        _step = 0;
    case 0:
        if (PREAMBLE1 == data)
            _step++;
        break;
    case 2:
        _step++;
        _class = data;
        _ck_b = _ck_a = data;   // reset the checksum accumulators
        break;
    case 3:
        _step++;
        _ck_b += (_ck_a += data);       // checksum byte
        _msg_id = data;
        break;
    case 4:
        _step++;
        _ck_b += (_ck_a += data);       // checksum byte
        _payload_length = data; // payload length low byte
        break;
    case 5:
        _step++;
        _ck_b += (_ck_a += data);       // checksum byte
        _payload_length += (uint16_t) (data << 8);
        if (_payload_length > 512) {
            _payload_length = 0;
            _step = 0;
        }
        _payload_counter = 0;   // prepare to receive payload
        break;
    case 6:
        _ck_b += (_ck_a += data);       // checksum byte
        if (_payload_counter < sizeof(_buffer)) {
            _buffer.bytes[_payload_counter] = data;
        }
        if (++_payload_counter == _payload_length)
            _step++;
        break;
    case 7:
        _step++;
        if (_ck_a != data)
            _step = 0;          // bad checksum
        break;
    case 8:
        _step = 0;
        if (_ck_b != data)
            break;              // bad checksum
        gpsData.present = 1;
        if (gpsParseUBLOX()) {
            parsed = true;
        }
    }                           //end switch
    return parsed;
}

static bool gpsParseUBLOX(void)
{
    switch (_msg_id) {
    case MSG_POSLLH:
        gpsData.lon = _buffer.posllh.longitude;
        gpsData.lat = _buffer.posllh.latitude;
        gpsData.height = _buffer.posllh.altitude_msl / 10 / 100;        //alt in m
        gpsData.fix = next_fix;
        _new_position = true;
        break;
    case MSG_STATUS:
        next_fix = (_buffer.status.fix_status & NAV_STATUS_FIX_VALID) && (_buffer.status.fix_type == FIX_3D);
        if (!next_fix)
            gpsData.fix = false;
        break;
    case MSG_SOL:
        next_fix = (_buffer.solution.fix_status & NAV_STATUS_FIX_VALID) && (_buffer.solution.fix_type == FIX_3D);
        if (!next_fix)
            gpsData.fix = false;
        gpsData.numSatellites = _buffer.solution.satellites;
        //GPS_hdop                        = _buffer.solution.position_DOP;
        //debug[3] = GPS_hdop;
        break;
    case MSG_VELNED:
        //speed_3d                        = _buffer.velned.speed_3d;  // cm/s
        gpsData.speed = _buffer.velned.speed_2d;        // cm/s
        gpsData.heading = (uint16_t) (_buffer.velned.heading_2d / 10000);       // Heading 2D deg * 100000 rescaled to deg * 10
        _new_speed = true;
        break;
    default:
        return false;
    }

    // we only return true when we get new position and speed data
    // this ensures we don't use stale data
    if (_new_position && _new_speed) {
        _new_speed = _new_position = false;
        return true;
    }
    return false;
}














void gpsTask(void *unused)
{
    gpsData.serial = serialOpen(USART1, 9600);
    gpsData.mode = MODE_NMEA;   // NMEA

    while (1) {
        CoTickDelay(25);

        while (uartAvailable(gpsData.serial)) {
            switch (gpsData.mode) {
            case MODE_NMEA:    // NMEA
                gpsData.validFrames += gpsNewFrameNMEA(uartRead(gpsData.serial));
                break;

            case MODE_UBX:     // UBX
                gpsData.validFrames += gpsNewFrameUBLOX(uartRead(gpsData.serial));
                break;

            case MODE_MTK:     // MTK
                break;

            case MODE_PASSTHROUGH:     // GPS -> UART bridge
                // TODO
                // usbSerialWrite(uartRead(gpsData.serial));
                break;
            }
        }
    }
}

void gpsInit(void)
{
    const uint8_t conf[] = { 0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xC8, 0x00, 0x01, 0x00, 0x01, 0x00, 0xDE, 0x6A };      // set rate to 5Hz
    const uint8_t conf_[] = { 0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0x01, 0x02, 0x01, 0x0E, 0x47 };       // set POSLLH MSG
    const uint8_t conf__[] = { 0xB5, 0x62, 0x06, 0x16, 0x08, 0x00, 0x03, 0x07, 0x03, 0x00, 0x51, 0x08, 0x00, 0x00, 0x8A, 0x41 };        // set EGNOS

#if 0
    timerDelay(1000);
    confGPS(conf, sizeof(conf));
    timerDelay(100);
    confGPS(conf_, sizeof(conf_));
    confGPS(conf__, sizeof(conf__));
#endif
    // STATUS, SOL, VELNED are ON by default

    CoCreateTask(gpsTask, 0, 20, &gpsStack[GPS_STACK_SIZE - 1], GPS_STACK_SIZE);
}
