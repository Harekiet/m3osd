//Make sure the buffer is always large enough

#include "libformat.h"

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

//Convert a single char to a digit
static int a2d(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    else if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    else
        return -1;
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

void formatOutput( PutFunction putFunc, void* putData , const char *fmt, va_list va) {
    //Temporary buffer for formatting numbers
	char buf[ FLOATLENGTH ];
	char ch;
    while ( (ch = *(fmt++)) ) {
    	//Check for magic marker
        if (ch != '%') {
        	putFunc( putData, ch );
        } else {
        	const char* write;
        	//Leading zero enabled
            unsigned char lz = 0;
           //How many digits to show, default will depend type
            char digits = -1;
            //Minimum width
            unsigned int minWidth = 0;
            ch = *(fmt++);
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
            while (minWidth-- > 0)
                putFunc( putData, fillChar );

            //Copy the rest of the string
            while ( (ch = *write++) )
            	putFunc( putData, ch );
        }
    }
}

typedef struct  {
	char* buffer;
	unsigned int left;
} StringData;

static void stringOutput( void* d, char ch ) {
	StringData* data = (StringData*)d;

	if ( data->left ) {
		*data->buffer++ = ch;
	}

}

char* formatString( char* outBuf, unsigned int outSize, const char *fmt, ... ) {
	StringData data;

	data.buffer = outBuf;
	data.left = outSize - 1;

    va_list va;
    va_start(va, fmt);
    formatOutput( stringOutput, &data, fmt, va );
    va_end(va);
    //Finish the string
    data.buffer[0] = 0;

    return outBuf;
}
