/*	This module assumes:
	1) At least 8 bits per char
	2) If 9 or more bits per char:
	   a) signed characters (for sign extension)
	   b) signed shift right
*/

#include <stdlib.h>
#include <money.h>
#include <ftype.h>
#include <errenv.h>

#define	SBIT	(((unsigned char)(~0)>>1)+1)

char signmethod() {
  char *s = getenv("MONEY");
  if (s) {
    switch (*s) {
    case 'O': case 'C':		/* OLD / COMPAT */
      return SBIT;		/* invert sign bit for sorting */
    case 'N':
      return 0;
    }
  }
  errdesc("MONEY",320);	/* must have environment */
}

static char sbit = 1;

MONEY ldM(p)   /* convert from file format to machine MONEY */
  FMONEY p;
{
  MONEY temp;
  if (sbit == 1) sbit = signmethod();
  *p ^= sbit;	/* invert sign for sorting */
  temp.high = ldlong(p);
  *p ^= sbit;	/* invert sign for sorting */
  temp.low  = ldshort(p+4);
  return temp;
}

char *stM(v,p)	/* convert MONEY from memory to file format */
  MONEY v; FMONEY p;  /* send MONEY to postion p */
{
  if (sbit == 1) sbit = signmethod();
  stlong(v.high,p);
  stshort(v.low,p+4);
  *p ^= sbit;	/* invert sign for sorting */
  return p;
}
