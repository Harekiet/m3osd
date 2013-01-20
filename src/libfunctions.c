#include "board.h"

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

//Fake errno to make the linker happy
int __errno;

//Very simplistic memset replacement
void * memset ( void * ptr, int value, int count ) {
	char *write = (char *)ptr;

	for (; count >0; --count ) {
		*write++ = value;
	}
	return ptr;
}



