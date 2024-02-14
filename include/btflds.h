#ifndef _BTFLDS_H
#define _BTFLDS_H
#ifdef __cplusplus
extern "C" {
#endif
/* 	Field table stored in the directory user area

    This file is part of the BTAS client library.

    The BTAS client library is free software: you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of the License,
    or (at your option) any later version.

    BTAS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with BTAS.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <btas.h>

enum { MAXFLDS = 200 };

struct btfrec {
  char type;		/* field type code */
  unsigned char len;	/* field length */
  short pos;		/* position in uncompressed record */
};		

struct btflds {
  unsigned char klen;	/* number of key fields */
  unsigned char rlen;	/* number of fields */
  /* variable length array, terminated by 0 type code */
  struct btfrec f[MAXFLDS];
};

/** BTAS field type codes.
   BT_CHAR fields store only printable characters.  Trailing blanks
   are compressed using a zero terminator.  Leading blanks are compressed
   by storing control codes less than blank - one less for each extra blank.
   E.g. in ascii, 1 blank = 0x20, 2 blanks = 0x1f, etc.

   BT_NUM fields can be from 1 - 6 bytes.  They are stored as twos
   complement integers with the sign bit inverted.  There are 16 type codes
   used to represent from 0 - 15 decimal digits after an implied decimal
   point.

   BT_DATE fields store days.  Time standard is implied, but
	is usually Earth Solar Time.
     2 bytes store days since 1901.
     4 bytes store julian day number.

   BT_TIME fields store seconds.  Time zone is implied, but is usually GMT.
     4 bytes store seconds since 1970.
     6 bytes store julian seconds.

   BT_REL fields are related to fields in a master file.  The len
   is interpreted as field #.

   Additional types may be defined by applications and will be displayed
   as hexadecimal by standard utilities.  BT_BIN is a standard
   hexadecimal type.
*/

enum {
  BT_CHAR = 1,
  BT_DATE = 2,
  BT_TIME = 3,
  BT_BIN = 4,		/* binary data */
  BT_PACK = 5,		/* packed binary data */
  BT_RECNO = 6,		/* record handle field */
  BT_FLT = 7,		/* IEEE float modified for unsigned sort */
  BT_RLOCK = 8,		/* record locking byte */
  BT_SEQ = 9,		/* sequence id for multi-part records */
  BT_BITS = 10,		/* null value map (1-bit for not-null fields) */
  BT_EBCDIC = 11,	/* EBCDIC character field */
  BT_JULMIN = 12,	/* minutes precision timestamp (minutes since 1970) */
  BT_REL = 0x20, /* - 0x2f */	/* related fields */
  BT_VNUM = 0x30,	/* compressed fixed point binary */
  BT_NUM = 0x40 /* - 0x4f */	/* fixed decimal point binary */
};

void load_rec(char *, struct btflds *, char *, int);
int store_rec(char *, struct btflds *, char *);
struct btflds *ldflds(struct btflds *, const char *, int);
int stflds(const struct btflds *, char *);
int btcreate(const char *, const struct btflds *, int);
BTCB *btopentmp(const char *, const struct btflds *, char *);
void btclosetmp(BTCB *, const char *);
void b2urec(const struct btfrec *, char *, int, const char *, int);
void u2brec(const struct btfrec *, const char *, int, BTCB *, int);
int packstr(char *,const char *,int);

#define BYTE unsigned char
void e2brec_begin(unsigned);
int e2brec_conv(BYTE *,const BYTE *,int,int);
unsigned e2brec(const BYTE *, const BYTE *, unsigned, BTCB *, int);
void b2erec(const BYTE *, BYTE *, int, const char *, int);
extern BYTE e2brec_skip;
#ifdef __cplusplus
}
#endif
#if defined(__cplusplus) || defined(__GNUC__)
static inline int btrlen(const struct btflds *fcb) {
  return fcb->f[fcb->rlen].pos;
}
#else
#define btrlen(fcb)	((fcb)->f[(fcb)->rlen].pos)
#endif
#endif
