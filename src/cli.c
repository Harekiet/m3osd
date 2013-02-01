/*
 * cli.c
 *
 *  Created on: 28 Jan 2013
 *      Author: Sjoerd
 */

#include "board.h"

//Main config for the program

static void cliOutput( void* data, char c ) {

}

void cliPrint( const char* fmt, ... ) {
	va_list va;
    va_start(va, fmt);
    formatOutput( cliOutput, 0, fmt, va );
    va_end(va);
};
