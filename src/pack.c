#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

#include "pack.h"

#if defined(_M_IX86) || defined(__i386__)
#define x86_32
/* x86 64 bit architecture */
#elif defined(__x86_64__) || defined(_M_X64)
#define x86_64
#endif
/*
 ** packi16() -- store a 16-bit int into a char buffer (like htons())
 */
void packi16(unsigned char *buf, unsigned short i)
{
	*buf++ = i>>8; *buf++ = i;
}

/*
 ** packi32() -- store a 32-bit int into a char buffer (like htonl())
 */
void packi32(unsigned char *buf, unsigned int i)
{
	*buf++ = i>>24; *buf++ = i>>16;
	*buf++ = i>>8;  *buf++ = i;
}

/*
 ** packi64() -- store a 64-bit int into a char buffer.
 * On 32-bit architectues long is 32-bit.
 * On 64-bit architectures long is 64-bit.
 */
void packi64(unsigned char *buf, unsigned long i)
{
#ifdef x86_64
	*buf++ = i>>56; *buf++ = i>>48;
	*buf++ = i>>40; *buf++ = i>>32;
	*buf++ = i>>24; *buf++ = i>>16;
	*buf++ = i>>8;  *buf++ = i;
#else /*x86_32*/
	/* Force 64-bit data zeroing the most significant part. Note the big-endianess */
	memset(buf, 0, 4);
	buf += 4;
	*buf++ = i>>24; *buf++ = i>>16;
	*buf++ = i>>8;  *buf++ = i;
#endif
}

/*
 ** unpacki16() -- unpack a 16-bit int from a char buffer (like ntohs())
 */
unsigned short unpacki16(unsigned char *buf)
{
	return (buf[0]<<8) | buf[1];
}

/*
 ** unpacki32() -- unpack a 32-bit int from a char buffer (like ntohl())
 */
unsigned int unpacki32(unsigned char *buf)
{
	return (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3];
}

/*
 ** unpacki64() -- unpack a 64-bit int from a char buffer
 */
unsigned long unpacki64(unsigned char *buf)
{
#ifdef x86_64
	return (buf[0]<<56) | (buf[1]<<48) | (buf[2]<<40) | (buf[3]<<32) |
			(buf[4]<<24) | (buf[5]<<16) | (buf[6]<<8) | buf[7];
#else /*x86_32*/
	/* Discard the most significant part. Note the big-endianess */
	return (buf[0]<<24) | (buf[5]<<16) | (buf[6]<<8) | buf[7];
#endif
}

/*
 ** pack() -- store data dictated by the format string in the buffer
 **
 **  h - 16-bit              l - 32-bit
 **  c - 8-bit char          z - 64-bit long
 ** s - string (16-bit length is automatically prepended)
 */
int32_t pack(unsigned char *buf, char *format, ...)
{
	va_list ap;
	int16_t h;
	int32_t l;
	int64_t z;
	int8_t c;
	char *s;
	int32_t size = 0, len;

	va_start(ap, format);

	for(; *format != '\0'; format++) {
		switch(*format) {
			case 'h': // 16-bit
				size += 2;
				h = (int16_t)va_arg(ap, int); // promoted
				packi16(buf, h);
				buf += 2;
				break;

			case 'l': // 32-bit
				size += 4;
				l = va_arg(ap, int32_t);
				packi32(buf, l);
				buf += 4;
				break;

			case 'z': // 64-bit
				size += 8;
				z = va_arg(ap, int64_t);
				packi64(buf, z);
				buf += 8;
				break;

			case 'c': // 8-bit
				size += 1;
				c = (int8_t)va_arg(ap, int); // promoted
				*buf++ = (c>>0)&0xff;
				break;

			case 's': // string
				s = va_arg(ap, char*);
				len = strlen(s);
				size += len + 2;
				packi16(buf, len);
				buf += 2;
				memcpy(buf, s, len);
				buf += len;
				break;
		}
	}

	va_end(ap);

	return size;
}

/*
 ** unpack() -- unpack data dictated by the format string into the buffer
 */
void unpack(unsigned char *buf, char *format, ...)
{
	va_list ap;
	int16_t *h;
	int32_t *l;
	int32_t pf;
	int64_t *z;
	int8_t *c;
	char *s;
	int32_t len, count, maxstrlen=0;

	va_start(ap, format);

	for(; *format != '\0'; format++) {
		switch(*format) {
			case 'h': // 16-bit
				h = va_arg(ap, int16_t*);
				*h = unpacki16(buf);
				buf += 2;
				break;

			case 'l': // 32-bit
				l = va_arg(ap, int32_t*);
				*l = unpacki32(buf);
				buf += 4;
				break;

			case 'z': // 64-bit
				z = va_arg(ap, int64_t*);
				*z = unpacki64(buf);
				buf += 8;
				break;

			case 'c': // 8-bit
				c = va_arg(ap, int8_t*);
				*c = *buf++;
				break;

			case 's': // string
				s = va_arg(ap, char*);
				len = unpacki16(buf);
				buf += 2;

				if (maxstrlen > 0 && len > maxstrlen) count = maxstrlen - 1;
				else count = len;

				memcpy(s, buf, count);
				s[count] = '\0';
				buf += len;
				break;

			default:
				if (isdigit(*format)) { // track max str len
					maxstrlen = maxstrlen * 10 + (*format-'0');
				}
		}

		if (!isdigit(*format)) maxstrlen = 0;
	}

	va_end(ap);
}
