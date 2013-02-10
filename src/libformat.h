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
void formatOutputVA( PutFunction putF, void* putD , const char *fmt, va_list va);
void formatOutput( PutFunction putF, void* putD , const char *fmt, ... );

//Format to a buffer returning it afterwards
char* formatString( char* outBuf, unsigned int outSize, const char *fmt, ... );

const char* extractWord( char ** cmd );

//Convert a string to an integeter, result should be an int32 or uint32 depending on sign
char makeInt( const char *p, char us, void *result );

//Convert a single char to a digit
//-1 when illegal
int a2d(char ch);


#endif /* LIBFORMAT_H_ */
