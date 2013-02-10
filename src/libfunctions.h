/*
 * libfunctions.h
 *
 *  Created on: 20 Jan 2013
 *      Author: Sjoerd
 */

#pragma once

#include <stddef.h>

#ifndef LIBFUNCTIONS_H_
#define LIBFUNCTIONS_H_

//#define NULL 0

extern void abort ( void );
extern void * memset ( void * ptr, int value, size_t count );
extern void * memcpy ( void * destination, const void * source, size_t num );

extern int strcmp( const char* str1, const char* str2 );
extern int strncasecmp( const char* str1, const char* str2, uint32_t max );

static inline int strcasecmp( const char* str1, const char* str2 ) {
	return strncasecmp( str1, str2, ~0 );
}

static inline int abs( int input ) {
	return input >= 0 ? input : -input;
}


#endif /* LIBFUNCTIONS_H_ */
