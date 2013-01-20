/*
 * libfunctions.h
 *
 *  Created on: 20 Jan 2013
 *      Author: Sjoerd
 */

#pragma once

#ifndef LIBFUNCTIONS_H_
#define LIBFUNCTIONS_H_


#define NULL 0

extern void * memset ( void * ptr, int value, int count );

static inline int abs( int input ) {
	return input >= 0 ? input : -input;
}


#endif /* LIBFUNCTIONS_H_ */
