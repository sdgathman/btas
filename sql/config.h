/* Sorting by floating numbers assumes the following:
	signed magnitude: msb - sign,exponent (excess notation),mantissa - lsb
   if floating numbers are stored lsb first, define SWAPDBL */
#ifdef i386
#define SWAPDBL		/* native floating point byte swapped */
#endif

#define RANGE	/* enable ranges ([A-Z]) for the LIKE operator */

/* #define USEDWORD	/* type 'D' in data dict is "DWORD", not "DATE" */

/* #define NOEBCDIC	/* turn off S/1 EBCDIC support */

/* #define mmddyy mdy	/* special MEDICOMP date structure */

/* #define const		/* Microsoft totally screws const */

extern const char version[];
