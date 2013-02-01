/*
 * libformat.h
 *
 *  Created on: 21 Jan 2013
 *      Author: Sjoerd
 */

#ifndef LIBFORMAT_H_
#define LIBFORMAT_H_

#include <stdarg.h>

typedef void (*PutFunction) (void *, char);

//Format to a function handling the character output
void formatOutput( PutFunction putF, void* putD , const char *fmt, va_list va);

//Format to a buffer returning it afterwards
char* formatString( char* outBuf, unsigned int outSize, const char *fmt, ... );


#endif /* LIBFORMAT_H_ */
