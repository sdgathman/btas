/*	%M% %I% %H%
	convert long to/from file format 
	This module assumes:
	1) At least 8 bits per char
	2) If 9 or more bits per char:
	   a) signed characters (for sign extension)
	   b) signed shift right
	You can do much better than these generic routines
	on CISC processors.  68k and x86 assembler versions are
	supplied.  For RISC, these are already fairly optimal.
*/

#undef m68k	/* so we don't get m68k macros */
#include "ftype.h"

#define ldw(p)	(((unsigned char)(p)[0]<<8)|(unsigned char)(p)[1])
#define lds(p)	(((signed char)(p)[0]<<8)|(unsigned char)(p)[1])

long ldlong(const char p[4]) {
  //return (signed char)p[0] << 24 | (unsigned char)p[1] << 16
   // 	| (unsigned char)p[2] << 8 | (unsigned char)p[3];
  return ((p[0] << 8 | (unsigned char)p[1]) << 8
      	| (unsigned char)p[2]) << 8 | (unsigned char)p[3];
  //return ((long)lds(p)<<16)|(unsigned short)ldw(p+2);
}

void stlong(long v, char p[4]) {
  p[3] = v;
  p[2] = v>>8;
  p[1] = v>>16;
  p[0] = v>>24;
}

short ldshort(const char p[2]) {
  return lds(p);
}

void stshort(int v,char p[2]) {
  p[1] = v;
  p[0] = v>>8;
}
