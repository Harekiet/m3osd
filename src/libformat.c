//Make sure the buffer is always large enough

#include "libformat.h"
#include <stdint.h>

#define FORMAT_FLOAT_DIGITS 7

//Need enough space for float
//1 negative indicator
//10 digits for 32bit number
//1 decimal point
//PRINTF_FLOAT_DIGITS fraction digits
//0 terminator

#define FLOATLENGTH (1 + 10 + 1 + FORMAT_FLOAT_DIGITS + 1)

//Return pointing at the 0 ending the string
static char* ui2a(unsigned int num, unsigned int base, int uc, char *bf)
{
    int n = 0;
    //Check how many steps to take
    unsigned int d = 1;
    while (num / d >= base)
        d *= base;
    while (d != 0) {
        int dgt = num / d;
        num %= d;
        d /= base;
        if (n || dgt > 0 || d == 0) {
            *bf++ = dgt + (dgt < 10 ? '0' : (uc ? 'A' : 'a') - 10);
            ++n;
        }
    }
    *bf = 0;
    return bf;
}

//Integer to a string, return end of string
static char* i2a(int num, char *bf)
{
    if (num < 0) {
        num = -num;
        *bf++ = '-';
    }
    return ui2a(num, 10, 0, bf);
}

//Convert a float to a string without using any floating point operations
static void f2a( float input, unsigned int digits, char *bf ) {
	union {
		long tempL;
		float tempF;
	} temp;
	temp.tempF = input;
	//temp value just to make me happy
	long val = temp.tempL;
	//Extract mantissa the 1.xxxxx value
	unsigned long mantissa = (val & 0xffffff) | 0x800000;
	//Extract the base 2 exponent
	int exp2 = ( ( val >> 23) & 0xff ) - 127;
	//Check exp2 for value we can't handle to indicate errors
	if ( exp2 < -23 || exp2 >= 31 ) {
		//yes a # indicates a float error!
		bf[0] = '#';
		bf[1] = 0;
		return;
	}
	//Create the integer and fractional part of the mantissa
	unsigned long intPart, fracPart;

	if (exp2 >= 23) {
		intPart = mantissa << (exp2 - 23);
		fracPart = 0;
	} else if (exp2 >= 0) {
	    intPart = mantissa >> (23 - exp2);
	    fracPart = (mantissa << (exp2 + 1)) & 0xFFFFFF;
	} else {
		 /* if (exp2 < 0) */
		intPart = 0;
		fracPart = (mantissa & 0xFFFFFF) >> -(exp2 + 1);
	}
	//negative start with a zero
	if( val < 0 )
		*bf++ = '-';
	//Output the integer part
	bf = ui2a( intPart, 10, 0, bf );
	//Nothing to do behind the decimal point
	if ( !digits )
		return;
	//output the fractional part
	*bf++ = '.';
	//check for max digit count
	if ( digits > FORMAT_FLOAT_DIGITS)
		digits = FORMAT_FLOAT_DIGITS;
	for( ; digits >0; digits-- ) {
		//Fraction makes up the lower 24 bits
		//Multiply it up to create the digits
		fracPart *= 10;
		*bf++ = ( fracPart >> 24 ) + '0';
		fracPart &= 0xFFFFFF;
	}
	//Finish string
	*bf = 0;
}

int a2d(char ch) {
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    else if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    else
        return -1;
}


//Try to make an integer from a string, signed or unsigned
char makeInt( const char *p, char us, void *result ) {
	char ch;
	uint32_t value = 0;
//	uint32_t max;
	unsigned int count = 0;
	unsigned int base;
	uint8_t negative = 0;

	// Skip leading white spaces
    while ( (*p == ' ') || (*p == '\t') ) {
        ++p;
    }

	//Check for a negative value
    if ( *p == '-' ) {
    	//Don't support negative with unsigned
    	if ( us )
    		return 0;
    	negative = 1;
    	++p;
    }

    //Check for hexadecimal input
    if ( *p == 'x' || *p == 'X' ) {
    	base = 16;
    	++p;
//		max = (( 1<< 32) - 16) / 16;
    //Check for binary input
	} else if ( *p == 'b' || *p == 'B' ) {
		base = 2;
    	++p;
//		max = (( 1<< 32) - 2) / 2;
	//Regular old decimal input
    } else {
    	base = 10;
//		max = (( 1<< 32) - 10) / 10;
    }

	for ( ch = *p; ch; p++, ch = *p, count++ ) {
		unsigned int add = a2d( ch );
		//Easy check against base and -1
		if ( add >= base ) {
			return 0;
		}
		//TODO check if you'd overflow the value?
		value = value * base + add;
	}
	//TODO check for additional chars?
	if ( count == 0 )
		return 0;

	if ( us ) {
		*(uint32_t*)result = value;
	} else if ( negative ) {
		//TODO check for signed limits
		*(int32_t*)result = -(int32_t)value;
	} else {
		//TODO check for signed limits
		*(int32_t*)result = (int32_t)value;
	}
	return 1;
}

//Extract a space separated word by modifying the original string
const char* extractWord( char ** cmd ) {
	char* start = *cmd;
	char c;
	while ( 1 ) {
		c = *start;
		//End of string return 0 that all is over
		if ( !c ) {
			*cmd = start;
			return 0;
		}
		//Any non white space interrupt scanning
		if ( c != ' ' && c != '\t' )
			break;
		++start;
	}
	//Check when the word ends
	char* end = start;
	while ( 1 ) {
		c = *end;
		//End of input
		if ( c == 0 ) {
			//Set the input the same
			*cmd = end;
			break;
		//hitting a white space, replace it with 0
		} else if ( c == ' ' || c == '\t' ) {
			//Replace the white space with a line terminator
			*end = 0;
			//input 1 beyond this for next word
			*cmd = end + 1;
			break;
		}
		++end;
	}
	return start;
}


//Convert a string to an number
static char a2ui(char ch, const char **src, int base, unsigned int *nump)
{
    const char *p = *src;
    int num = 0;
    int digit;
    while ((digit = a2d(ch)) >= 0) {
        if (digit > base)
            break;
        num = num * base + digit;
        ch = *p++;
    }
    *src = p;
    *nump = num;
    return ch;
}

void formatOutputVA( PutFunction putFunc, void* putData, const char *fmt, va_list va) {
    //Temporary buffer for formatting numbers
	char buf[ FLOATLENGTH ];
	char ch;
    while ( (ch = *(fmt++)) ) {
    	//Check for magic marker
        if (ch != '%') {
        	putFunc( putData, ch );
        } else {
        	const char* write;
        	unsigned char rightJustify = 1;
        	//Leading zero enabled
            unsigned char lz = 0;
           //How many digits to show, default will depend type
            char digits = -1;
            //Minimum width
            unsigned int minWidth = 0;
            ch = *(fmt++);
            //Check for left justify
            if ( ch == '-' ) {
            	ch = *(fmt++);
            	rightJustify = 0;
            }
            //Check for 0 fill
            if (ch == '0') {
                ch = *(fmt++);
                lz = 1;
            }
            //Check for any leading digits indicating minimum width
            if (ch >= '0' && ch <= '9') {
                ch = a2ui(ch, &fmt, 10, &minWidth);
            }
            //Decimal separator for indicating digits
            if ( ch == '.' ) {
                ch = *(fmt++);
            	//Simple check only supporting 0-9 digits
                if (ch >= '0' && ch <= '9') {
                	digits = ( ch - '0' );
                    ch = *(fmt++);
                }
            }
            switch (ch) {
            case 0:
               return;
            case 'u':
                ui2a(va_arg(va, unsigned int), 10, 0, buf );
                write = buf;
                break;
            case 'd':
            	i2a(va_arg(va, int), buf );
                write = buf;
            	break;
            case 'x':
            case 'X':
           		ui2a(va_arg(va, unsigned int), 16, (ch == 'X'), buf );
           		write = buf;
           		break;
            case 'c':
            	buf[0] = (char) (va_arg(va, int) );
            	buf[1] = 0;
            	write = buf;
                break;
            case 's':
            	write = va_arg(va, char *);
                break;
            case 'f':
            	//float will always get passed as double as an argument
            	f2a( va_arg(va, double), digits >= 0 ? digits : FORMAT_FLOAT_DIGITS, buf );
            	write = buf;
                break;
            case '%':
            	buf[0] = ch;
            	buf[1] = 0;
            	write = buf;
            	break;
            //Skip the rest
            default:
            	continue;
            }
            //output the string keeping track of leading white spaces/zeros
            char fillChar = lz ? '0' : ' ';
            const char *getLen = write;
            while (*getLen++ && minWidth > 0)
                minWidth--;
            if ( rightJustify ) {
            	while (minWidth-- > 0)
            		putFunc( putData, fillChar );
            }

            //Copy the rest of the string
            while ( (ch = *write++) )
            	putFunc( putData, ch );

            if ( !rightJustify ) {
            	while (minWidth-- > 0)
            		putFunc( putData, fillChar );
            }

        }
    }
}

void formatOutput( PutFunction putFunc, void* putData, const char *fmt, ... ) {
    va_list va;
    va_start(va, fmt);
    formatOutputVA( putFunc, putData, fmt, va );
    va_end(va);
}

typedef struct  {
	char* buffer;
	unsigned int left;
} StringData;

static void stringOutput( void* d, char ch ) {
	StringData* data = (StringData*)d;

	if ( data->left ) {
		*data->buffer++ = ch;
		data->left--;
	}
}

char* formatString( char* outBuf, unsigned int outSize, const char *fmt, ... ) {
	StringData data;

	data.buffer = outBuf;
	data.left = outSize - 1;

    va_list va;
    va_start(va, fmt);
    formatOutputVA( stringOutput, &data, fmt, va );
    va_end(va);
    //Finish the string
    data.buffer[0] = 0;

    return outBuf;
}
