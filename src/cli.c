/*
 * cli.c
 *
 *  Created on: 28 Jan 2013
 *      Author: Sjoerd
 */

#include "board.h"
#include "multiwii.h"
#include <unwind.h>

//Forwards
static void cliHelp( char* args );

static void cliReset(bool toBootloader)
{
	#define AIRCR_VECTKEY_MASK    ((uint32_t)0x05FA0000)
	if (toBootloader) {
        // 1FFFF000 -> 20000200 -> SP
        // 1FFFF004 -> 1FFFF021 -> PC
        *((uint32_t *)0x20004FF0) = 0xDEADBEEF; // 20KB STM32F103
    }

    // Generate system reset
    SCB->AIRCR = AIRCR_VECTKEY_MASK | (uint32_t)0x04;
}

static void cliOutput( void* data, char c ) {
	uartWrite(multiwiiData.serial, c );
}

static void cliPrint( const char* fmt, ... ) {
	va_list va;
    va_start(va, fmt);
    formatOutputVA( cliOutput, 0, fmt, va );
    va_end(va);
}

static void cliPrintLine( const char* fmt, ... ) {
	va_list va;
    va_start(va, fmt);
    formatOutputVA( cliOutput, 0, fmt, va );
    va_end(va);
    uartWrite(multiwiiData.serial, '\r' );
    uartWrite(multiwiiData.serial, '\n' );
}

//Show the default prompt
static void cliPrompt() {
    cliPrint("\r\n# ");
}

static void cliClear() {
    // clear screen
    cliPrint("\033[2J\033[1;1H");
    cliPrompt();
}

void cliStart() {
	multiwiiData.rxIndex = 0;
	cliClear();
}

static void cliSet( char* args ) {
	const char* name = extractWord( &args );
	if ( name && ( name[0] != '*' ) ) {
		const ConfigEntry_t* entry = configFind( name );
		if ( !entry ) {
			cliPrintLine( "Entry %s not found", name );
			return;
		}
		const char* value = extractWord( &args );
		if ( value ) {
			uint8_t ret = configSetString( entry, value );
			switch( ret ) {
			case CONF_SET_FAIL:
				cliPrintLine( "Failed to set %s to %s", name, value);
				return;
				break;
			case CONF_SET_MIN:
			case CONF_SET_MAX:
				cliPrint( "Set %s clipped to ", entry->name );
				configPrint( entry, cliOutput, 0 );
				cliPrintLine( "" );
				break;
			case CONF_SET_FINE:
				cliPrint( "Set %s to ", entry->name );
				configPrint( entry, cliOutput, 0 );
				cliPrintLine( "" );
			}
			return;
		} else {

		}
	} else {
		cliPrintLine( "Current Settings:" );
		uint32_t index = 0;
		while ( 1 ) {
			const ConfigEntry_t* entry = configIndex( index++ );
			if ( !entry )
				break;
			cliPrint( "%s = ", entry->name );
			configPrint( entry, cliOutput, 0 );
			cliPrintLine( "" );
		}
	}

}

static void cliDefaults( char* args ) {

}

static void cliExit( char* args ) {
	cliReset( 0 );
}

static void cliSave( char* args ) {
	configSave();
	cliReset( 0 );
}

static void cliStatus( char* args ) {
	uint32_t seconds = CoGetOSTime() / 1000;
	cliPrintLine("Uptime %d seconds", seconds );
	cliPrintLine("CPU %d Mhz", SystemCoreClock / 1000000 );
}

static void cliVersion( char* args ) {
	cliPrint("OSDongs CLI version 1.0 " __DATE__ " / " __TIME__);
}

typedef struct {
    const char *name;
    const char *help;
    void (*func)(char *cmdline);
} clicmd_t;

// should be sorted a..z
static const clicmd_t cmdTable[] = {
    { "defaults", "reset to defaults and reboot", cliDefaults },
    { "exit", "reboot without saving", cliExit },
    { "help", "Show this help", cliHelp },
    { "save", "Save and reboot", cliSave },
    { "set", "Set and view configuration variables", cliSet },
    { "status", "Show system status", cliStatus },
    { "version", "Show firmware version", cliVersion },
};

#define CMD_COUNT ( sizeof( cmdTable ) / sizeof( cmdTable[0] ) )

static void cliHelp( char* args ) {
    uint32_t i = 0;

    cliPrintLine( "Available commands:" );
    for (i = 0; i < CMD_COUNT; i++) {
        cliPrintLine("%-9s  %s", cmdTable[i].name, cmdTable[i].help );
    }
}

void cliParse( char* input ) {
	const char* first = extractWord( &input );

	if ( first ) {
		uint32_t count = sizeof( cmdTable ) / sizeof( cmdTable[0] );
		const clicmd_t* cmd = cmdTable;
		for ( ; count > 0; count--, cmd++ ) {
			if ( !strcasecmp( first, cmd->name ) ) {
				cmd->func( input );
				return;
			}
		}
		//No mwatch show warning
		cliPrint( "Unknown command: %s", first );
	}

}

void cliReceive( char c ) {
	//Reuse the multiwii buffer
	switch( c ) {
	case 12:
        // clear screen
        cliClear();
        break;
	//end of line
	case 10:
	case 13:
		//Terminate string and have it parsed
		if ( multiwiiData.rxIndex ) {
			multiwiiData.buffer[multiwiiData.rxIndex++] = 0;
			cliPrintLine( "" );
			cliParse( (char*)multiwiiData.buffer );
			multiwiiData.rxIndex = 0;
			cliPrompt();
		}
		break;
	case 127:	//backspace
        // backspace
        if (multiwiiData.rxIndex) {
        	multiwiiData.rxIndex--;

            cliPrint("\010 \010");
        }
        break;
	case 32:
		//Ignore spaces if the line is empty
		if ( !multiwiiData.rxIndex )
			break;
	default:
		//Only add ascii chars when there's enough room in the buffer
		if ( c >= 32 && c <= 127 && multiwiiData.rxIndex < sizeof( multiwiiData.buffer) - 1 ) {
			multiwiiData.buffer[multiwiiData.rxIndex++] = c;
			cliOutput( 0, c );
		}
		break;
	}
}
