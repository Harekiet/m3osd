#include "board.h"

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

//Some fake empty variables to make gcc give up on stupid shit
const int __errno[0];
const char __aeabi_unwind_cpp_pr0[0];

//Very simplistic memset replacement
void * memset ( void * ptr, int value, size_t count ) {
	char *write = (char *)ptr;

	for (; count >0; --count ) {
		*write++ = value;
	}
	return ptr;
}


void * memcpy ( void * destination, const void * source, size_t count ) {
	char * read = (char*) source;
	char * write = (char *) destination;

	for (; count >0; --count ) {
		*write++ = *read++;
	}
	return destination;
}


void abort ( void ) {
	while ( 1 ) {
	}
}


int strcmp( const char* str1, const char* str2 ) {
	for ( ; ; str1++, str2++ ) {
		char c1 =*str1;
		char c2 = *str2;

		//Check for line endings on either string
		if ( !c1 ) {
			return c2 ? 1 : 0;
		} else if ( !c2 ) {
			return -1;
		}

		if ( c1 < c2 )
			return -1;
		if ( c1 > c2 )
			return 1;
	}
}

extern int strncasecmp( const char* str1, const char* str2, uint32_t max ) {
	for ( ; max; str1++, str2++, max-- ) {
		char c1 =*str1;
		char c2 = *str2;

		//Check for line endings on either string
		if ( !c1 ) {
			return c2 ? 1 : 0;
		} else if ( !c2 ) {
			return -1;
		}

		//Lowercase the case before comparing
		if ( c1 >= 'A' && c1 <= 'Z' )
			c1 += 32;
		if ( c2 >= 'A' && c2 <= 'Z' )
			c2 += 32;

		if ( c1 < c2 )
			return -1;
		if ( c1 > c2 )
			return 1;
	}
	//max triggered
	return 0;
}

#if 0
int __aeabi_idiv0 (int return_value) {
	abort();
	return 0;
}

long long __aeabi_ldiv0 (long long return_value) {
	abort();
	return 0;
}

#endif
