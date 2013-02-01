/*
 * config.c
 *
 *  Created on: 28 Jan 2013
 *      Author: Sjoerd
 */

#include "board.h"
#include "osdcore.h"

#ifndef FLASH_PAGE_COUNT
#define FLASH_PAGE_COUNT 128
#endif

#define FLASH_PAGE_SIZE                 ((uint16_t)0x400)
#define FLASH_WRITE_ADDR                (0x08000000 + (uint32_t)FLASH_PAGE_SIZE * (FLASH_PAGE_COUNT - 1))       // use the last KB for storage

#define CONFIG_VERSION 1

Config_t cfg;


//Simple slow crc16 calculation without lookup table
static uint16_t makeCRC16(const uint8_t* input, uint32_t size) {
	enum {
		WIDTH = 16,
		TOPBIT = (1 << WIDTH) - 1,
		POLYNOMIAL = 0x8810
	};

	uint16_t remainder = 0;

	for (uint32_t i = 0; i < size; i++) {
		//Feed new byte into the value
		remainder ^= input[i] << (WIDTH - 8);

		/*
		 * Perform modulo-2 division, a bit at a time.
		 */
		for (uint8_t bit = 8; bit > 0; --bit) {
			/*
			 * Try to divide the current data bit.
			 */
			if (remainder & TOPBIT) {
				remainder = (remainder << 1) ^ POLYNOMIAL;
			} else {
				remainder = (remainder << 1);
			}
		}
	}
	return remainder;
}


static uint16_t configCRC16( const Config_t* input ) {
	//Always begin right after the crc in the beginning
	const uint8_t* start = (uint8_t*)( &input->crc16 + 1 );
	const uint8_t* end = (uint8_t*)( input + 1 );

	uint32_t size = end - start;
	return makeCRC16( start, size );
}


//Check for a valid configuration in memory
static uint8_t validFlash() {
    const Config_t *temp = (const Config_t *)FLASH_WRITE_ADDR;

    // check version number
    if ( CONFIG_VERSION != temp->version)
        return 0;

    // check size
    if (temp->size != sizeof( *temp ) )
        return 0;

    //Check the crc16
    if ( configCRC16( temp ) != temp->crc16 )
    	return 0;
    // looks good, let's roll!
    return 1;
}

static void readFlash() {
	cfg = *( (const Config_t *)FLASH_WRITE_ADDR);
}

static void writeFlash() {
    FLASH_Status status;
    uint32_t i;

    //Finish the config to be written to the flash
    cfg.version = CONFIG_VERSION;
    cfg.size = sizeof( cfg );
    cfg.crc16 = configCRC16( &cfg );

#if 0
    //TODO Maybe lock coos
    // write it
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    if (FLASH_ErasePage(FLASH_WRITE_ADDR) == FLASH_COMPLETE) {
        for (i = 0; i < sizeof(cfg); i += 4) {
            status = FLASH_ProgramWord(FLASH_WRITE_ADDR + i, *(uint32_t *) ((char *) &cfg + i));
            if (status != FLASH_COMPLETE)
                break;          // TODO: fail
        }
    }
    FLASH_Lock();
#endif

    //Do some flashing to indicate magic happening
    LED0_OFF;
    LED1_ON;
    for ( i = 0; i < 20; i++ ) {
    	LED0_TOGGLE;
    	LED1_TOGGLE;
    	//50 millisecond delay
//    	timerDelay( 50000 );
//		timerDelay( 50000 );
    	timerDelay( 50000 );
    }
    LED0_OFF;
    LED1_OFF;

}


void configLoad() {
	//Read the flash if it's valid
	if ( validFlash() ) {
		readFlash();
	} else {
		configReset();
	}

}

// Default settings
void configReset() {
    memset(&cfg, 0, sizeof(cfg) );

    cfg.version = CONFIG_VERSION;
    cfg.width = OSD_WIDTH_MAX;
    cfg.height = OSD_HEIGHT_MAX;
    cfg.delayX = 55;
    cfg.delayY = 35;
    cfg.showBorder = 0;
    cfg.clockDivider = 9;
    cfg.gpsBaudrate = 115200;

    writeFlash();
}

void configSave() {
	writeFlash();
}

