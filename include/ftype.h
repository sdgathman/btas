/*
	Basic portable file types
*/
#ifndef _FTYPE_H_
#define _FTYPE_H_
#ifdef __cplusplus
extern "C" {
#endif

typedef char FLONG[4], FSHORT[2], FFLOAT[4], FDBL[8];

#ifdef m68k 	/* m68k BIGENDIAN, alignment optional */

#define ldlong(a)    (*(long *)(a))
#define stlong(l,a)  (*(long *)(a) = l)
#define ldshort(a)   (*(short *)(a))
#define stshort(s,a) (*(short *)(a) = s)

#else

/* can't use typedef'ed arrays in prototypes because some
   compilers choke :-( */
long ldlong(const char[4]);
short ldshort(const char [2]);
void stlong(long, char [4]), stshort(int, char [2]);

#endif

void ldchar(const char *, int, char *);
int  stchar(const char *, char *, int);
/* stchar returns -1 if truncated */

/* "sort" strings */
#define CASEMASKSZ(s)	(((s-1)/8)+1)
#define SSTRSIZE(s)	(s-CASEMASKSZ(s))
extern char *stsstr(char *,int,char *,int);	/* src,slen,dst,dlen */
extern char *ldsstr(char *,int,char *,int);	/* dst,dlen,src,slen */

/* Floating types are IEEE, formatted for unsigned binary sorting. */
void stdbl(double, char [8]);
double lddbl(const char [8]);
void stfloat(double, char [4]);
float ldfloat(const char [4]);

/* Series/1 floating types are base 16 */
void sts1extfloat(double, char [8]);
double lds1extfloat(const char [8]);
void sts1float(double, char [4]);
double lds1float(const char [4]);

#ifdef __cplusplus
}
#endif
#endif
