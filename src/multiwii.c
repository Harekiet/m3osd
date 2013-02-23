#include "board.h"
#include "uart.h"
#include "multiwii.h"
#include "cli.h"
#include "usb.h"

#define MULTIWII_STACK_SIZE 128
OS_STK multiwiiStack[MULTIWII_STACK_SIZE] __attribute__ ((aligned (8)));

//Is the cli active or not
static uint8_t cliActive = 0;

multiwiiData_t multiwiiData;

static const uint8_t MSP_HEADER[3] = { 0x24, 0x4D, 0x3C };

uint8_t read8(void)
{
    return multiwiiData.buffer[multiwiiData.rdIndex++];
}

uint16_t read16(void)
{
    uint16_t t = read8();
    t += (uint16_t) read8() << 8;
    return t;
}

uint32_t read32(void)
{
    uint32_t t = read16();
    t += (uint32_t) read16() << 16;
    return t;
}

void requestMSP(uint8_t msp, uint8_t * payload, uint8_t size)
{
    int i;
    uint8_t checksum = 0;

    for (i = 0; i < 3; i++)
        uartWrite(multiwiiData.serial, MSP_HEADER[i]);

    uartWrite(multiwiiData.serial, size);
    checksum ^= size;
    uartWrite(multiwiiData.serial, msp);
    checksum ^= msp;

    for (i = 0; i < size; i++) {
        uartWrite(multiwiiData.serial, payload[i]);
        checksum ^= payload[i];
    }
    uartWrite(multiwiiData.serial, checksum);
}

void parseMSP(void)
{
    int i;

    multiwiiData.rdIndex = 0;
    multiwiiData.mspParseOK = 0;

    switch (multiwiiData.cmdMSP) {

    case MSP_ALTITUDE:
        multiwiiData.altitude = (int32_t) read32();
        break;

    case MSP_ATTITUDE:
        multiwiiData.angleRoll = (int16_t) read16();
        multiwiiData.anglePitch = (int16_t) read16();
        multiwiiData.heading = (int16_t) read16();
        break;

    case MSP_RAW_GPS:
        multiwiiData.GPS_FIX = (uint8_t) read8();
        multiwiiData.GPS_numSat = (uint8_t) read8();
        multiwiiData.GPS_LAT = (int32_t) read32();
        multiwiiData.GPS_LON = (int32_t) read32();
        multiwiiData.GPS_altitude = (uint16_t) read16();
        multiwiiData.GPS_speed = (uint16_t) read16();
        multiwiiData.GPS_update ^= 2;
        // multiwiiData.GPS_update++;   // & 2
        break;

    case MSP_COMP_GPS:
        multiwiiData.GPS_distanceToHome = (uint16_t) read16();
        multiwiiData.GPS_directionToHome = (uint16_t) read16();
        multiwiiData.GPS_update ^= 1;   //(uint8_t)read8() & 1;   // & 1
        break;

    case MSP_WP:
        read8();                // wp0 i.e. home
        multiwiiData.GPS_homeLAT = (int32_t) read32();
        multiwiiData.GPS_homeLON = (int32_t) read32();
        read16();               // altitude will come here
        read8();                // nav flag will come here
        multiwiiData.GPS_update ^= 4;
        break;

    default:
        for (i = 0; i < multiwiiData.dataSize; i++)
            read8();
        break;
    }
}

static void receiveMSP( uint8_t c) {
    static enum _serial_state {
        IDLE,
        HEADER_START,
        HEADER_M,
        HEADER_ARROW,
        HEADER_SIZE,
        HEADER_CMD,
    } c_state = IDLE;

	if (c_state == IDLE) {
		if ( c == '$' ) {
			c_state = HEADER_START;
		} else if ( c == '#' ) {
//			cliStart();
//			cliActive = 1;
		}
	} else if (c_state == HEADER_START) {
		c_state = (c == 'M') ? HEADER_M : IDLE;
	} else if (c_state == HEADER_M) {
		c_state = (c == '>') ? HEADER_ARROW : IDLE;
	} else if (c_state == HEADER_ARROW) {
		if (c > MSP_BUFFER_SIZE) {  // now we are expecting the payload size
			c_state = IDLE;
		} else {
			multiwiiData.dataSize = c;
			c_state = HEADER_SIZE;
		}
	} else if (c_state == HEADER_SIZE) {
		c_state = HEADER_CMD;
		multiwiiData.cmdMSP = c;
	} else if (c_state == HEADER_CMD) {
		multiwiiData.buffer[multiwiiData.rxIndex++] = c;
		if (multiwiiData.rxIndex >= multiwiiData.dataSize) {
			multiwiiData.rxIndex = 0;
			multiwiiData.mspParseOK = 1;
			c_state = IDLE;
		}
		if (multiwiiData.mspParseOK)
			parseMSP();
	}
}

static void multiwiiRequestData(int type)
{
    uint8_t wp = 0;

    switch (type) {  // req without spike
        default:
        case 0:
            requestMSP(MSP_ATTITUDE, NULL, 0);
            requestMSP(MSP_ALTITUDE, NULL, 0);
            break;
        
        case 1:
            requestMSP(MSP_RAW_GPS, NULL, 0);  
            break;

        case 2:
            requestMSP(MSP_COMP_GPS, NULL, 0);  // LOS, HOD
            break;
        
        case 3:
            requestMSP(MSP_WP, &wp, 1);   // Home Pos
            break;
    }
}

static void multiwiiTask(void *unused)
{
    int type = 0;
    
    multiwiiData.serial = serialOpen(USART2, 115200);

    while (1) {
        CoTickDelay(8);
        while( USB_RXAvailable() ) {
        	uint8_t c = USB_RXRead();
        	USB_TXWrite( c );
        }

        if ( !cliActive ) {
        	multiwiiRequestData(type++ % 4);
          	CoTickDelay(2);
          	while (uartAvailable(multiwiiData.serial))
          		receiveMSP( uartRead( multiwiiData.serial ) );
        } else {
          	while (uartAvailable(multiwiiData.serial)) {
          		cliReceive( uartRead( multiwiiData.serial ) );
          	}
        }
    }
}

void multiwiiInit(void)
{
    CoCreateTask(multiwiiTask, 0, 15, &multiwiiStack[MULTIWII_STACK_SIZE - 1], MULTIWII_STACK_SIZE);
}
