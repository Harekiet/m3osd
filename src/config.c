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

//Table of all the config entries and their limits and what not
static const ConfigEntry_t ConfigTable[] = {
	#include "configtable.h"
};

#define TABLE_COUNT (sizeof(ConfigTable) / sizeof(ConfigTable[0]))

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
    uint32_t i;
#if 0
    FLASH_Status status;
    //Finish the config to be written to the flash
    cfg.version = CONFIG_VERSION;
    cfg.size = sizeof( cfg );
    cfg.crc16 = configCRC16( &cfg );
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

const ConfigEntry_t* configIndex( uint32_t index ) {
    uint32_t count = TABLE_COUNT;
    if ( index < count ) {
    	return ConfigTable + index;
    } else {
    	return 0;
    }
}

const ConfigEntry_t* configFind( const char* match ) {
    uint32_t count = sizeof( ConfigTable ) / sizeof( ConfigTable[0] );
    const ConfigEntry_t* entry = ConfigTable;
    for( ; count > 0; entry++, count-- ) {
    	if ( !strcasecmp( match, entry->name ) )
    		return entry;
    }
    return 0;
}

#define PRINTENTRY( _TYPE, _FORMAT ) \
	case CONF_TYPE_ ## _TYPE :	\
		formatOutput( putFunc, putData, _FORMAT, *(CONF_TYPEDEF_ ## _TYPE *)entry->data );	\
		break;

void configPrint( const ConfigEntry_t* entry, PutFunction putFunc, void* putData ) {
	//Value to be set into the entry
	switch( entry->type ) {
	PRINTENTRY( U8, "%u" );
	PRINTENTRY( U16, "%u" );
	PRINTENTRY( U32, "%u" );
	PRINTENTRY( S8, "%d" );
	PRINTENTRY( S16, "%d" );
	PRINTENTRY( S32, "%d" );
	}
}

//Helper define to set a value and clip it against min/max values
#define SETENTRY( _TYPE,  _ENTRY, _VALUE ) \
	case CONF_TYPE_ ## _TYPE :	\
	if ( _ENTRY ->min. _TYPE || _ENTRY ->max. _TYPE ) {				\
		if ( _VALUE . _TYPE < _ENTRY ->min. _TYPE ) { *(CONF_TYPEDEF_ ## _TYPE *)_ENTRY ->data = _ENTRY ->min. _TYPE; return CONF_SET_MIN; }	\
		if ( _VALUE . _TYPE > _ENTRY ->max. _TYPE ) { *(CONF_TYPEDEF_ ## _TYPE *)_ENTRY ->data = _ENTRY ->max. _TYPE; return CONF_SET_MAX; }	\
	}	\
	*(CONF_TYPEDEF_ ## _TYPE *)_ENTRY ->data = _VALUE . _TYPE;	\
	return CONF_SET_FINE;

//Returns 1 when successful
uint8_t configSet( const ConfigEntry_t* entry, const ConfigValue_t value ) {
	switch ( entry->type ) {
	SETENTRY( U8, entry, value );
	SETENTRY( U16, entry, value );
	SETENTRY( U32, entry, value );
	SETENTRY( S8, entry, value );
	SETENTRY( S16, entry, value );
	SETENTRY( S32, entry, value );
	SETENTRY( FLT, entry, value );
	}
	return CONF_SET_FAIL;
}

uint8_t configSetString( const ConfigEntry_t* entry, const char* string ) {
	//Value to be set into the entry
	ConfigValue_t value;
	switch( entry->type ) {
	case CONF_TYPE_U8:
		if ( !makeInt( string, 1, &value.U32 ) )
			return CONF_SET_FAIL;
		value.U8 = value.U32;
		break;
	case CONF_TYPE_U16:
		if ( !makeInt( string, 1, &value.U32 ) )
			return CONF_SET_FAIL;
		value.U16 = value.U32;
		break;
	case CONF_TYPE_U32:
		if ( !makeInt( string, 1, &value.U32 ) )
			return CONF_SET_FAIL;
		break;
	case CONF_TYPE_S8:
		if ( !makeInt( string, 0, &value.S32 ) )
			return CONF_SET_FAIL;
		value.S8 = value.S32;
		break;
	case CONF_TYPE_S16:
		if ( !makeInt( string, 0, &value.S32 ) )
			return CONF_SET_FAIL;
		value.S16 = value.S32;
		break;
	case CONF_TYPE_S32:
		if ( !makeInt( string, 0, &value.S32 ) )
			return CONF_SET_FAIL;
		break;
	default:
		return CONF_SET_FAIL;
	}
	return configSet( entry, value );
}

// Default settings
void configReset() {
    memset(&cfg, 0, sizeof(cfg) );

    uint32_t count = TABLE_COUNT;
    //Go through my entire table resettings everything
    const ConfigEntry_t* entry = ConfigTable;
    for( ; count > 0; entry++, count-- ) {
    	configSet( entry, entry->reset );
    }
    writeFlash();
}

void configSave() {
	writeFlash();
}


