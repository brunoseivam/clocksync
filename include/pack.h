#ifndef _PACK_H_
#define _PACK_H_

/* ARCH dependent MACROS. GNU C and Visual Studio macro definitions*/
#if defined(__i386__) || defined(_M_IX86)
#define x86_32
/* x86 64 bit architecture */
#elif defined(__x86_64__) || defined(_M_X64)
#define x86_64
#endif

/* Routines for pack/unpack data */
/*
 ** packi16() -- store a 16-bit int into a char buffer (like htons())
 */ 
void packi16(unsigned char *buf, unsigned short i);

/*
 ** packi32() -- store a 32-bit int into a char buffer (like htonl())
 */ 
void packi32(unsigned char *buf, unsigned int i);

/*
 ** packi64() -- store a 64-bit int into a char buffer
 */ 
void packi64(unsigned char *buf, unsigned long i);

/*
 ** unpacki16() -- unpack a 16-bit int from a char buffer (like ntohs())
 */ 
unsigned short unpacki16(unsigned char *buf);

/*
 ** unpacki32() -- unpack a 32-bit int from a char buffer (like ntohl())
 */ 
unsigned int unpacki32(unsigned char *buf);

/*
 ** unpacki64() -- unpack a 64-bit int from a char buffer
 */ 
unsigned long unpacki64(unsigned char *buf);

int32_t pack(unsigned char *buf, char *format, ...);

/*
 ** unpack() -- unpack data dictated by the format string into the buffer
 */
void unpack(unsigned char *buf, char *format, ...);

#endif
