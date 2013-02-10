/*
 * config.h
 *
 *  Created on: 9 Feb 2013
 *      Author: Sjoerd
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdint.h>


typedef uint8_t CONF_TYPEDEF_U8;
typedef uint16_t CONF_TYPEDEF_U16;
typedef uint32_t CONF_TYPEDEF_U32;
typedef int8_t CONF_TYPEDEF_S8;
typedef int16_t CONF_TYPEDEF_S16;
typedef int32_t CONF_TYPEDEF_S32;
typedef float CONF_TYPEDEF_FLT;

typedef union {
	uint32_t U32;
	uint16_t U16;
	uint8_t U8;
	int32_t S32;
	int16_t S16;
	int8_t S8;
	float FLT;
} ConfigValue_t;

enum {
	CONF_TYPE_U8,
	CONF_TYPE_U16,
	CONF_TYPE_U32,
	CONF_TYPE_S8,
	CONF_TYPE_S16,
	CONF_TYPE_S32,
	CONF_TYPE_FLT,
};

//configset returns incase you want to show messages for going out of limits
enum {
	CONF_SET_FAIL = 0,
	CONF_SET_FINE = 1,
	CONF_SET_MIN = 2,
	CONF_SET_MAX = 3,
};

typedef struct {
	void* data;
	const char* name;
	ConfigValue_t reset;
	ConfigValue_t min;
	ConfigValue_t max;
	uint8_t type;
	uint8_t flags;
} ConfigEntry_t;

//Reset to default settings and save those
void configReset();
//Just save the current settings
void configSave();
//Reload the settings ignore any changes
void configLoad();

//Find a specific config entry
const ConfigEntry_t* configFind( const char* match );

//Get a specific config entry
const ConfigEntry_t* configIndex( uint32_t index );

uint8_t configSet( const ConfigEntry_t*, ConfigValue_t value );
uint8_t configSetString( const ConfigEntry_t*, const char* string );
void configPrint( const ConfigEntry_t* entry, PutFunction putFunc, void* putData );

//config.c
typedef struct {
	//16bit crc of all the following data based on the size
    uint16_t crc16;
    //Size of the entire config structure
	uint16_t size;

#define CONF_GLOBAL
	#include "configtable.h"
#undef CONF_GLOBAL

	//Version of the current configuration as an extra check that has to match
	uint8_t version;
} Config_t;

extern Config_t cfg;

#endif /* CONFIG_H_ */
