/*
 * cli.h
 *
 *  Created on: 28 Jan 2013
 *      Author: Sjoerd
 */

#ifndef CLI_H_
#define CLI_H_

void cliPrint( const char* fmt, ... );

void cliParse( char* cmd );

void cliStart();

void cliReceive( char c );


#endif /* CLI_H_ */
