/*
 * $Id$
 * Request format for cisam local server
 * $Log$
 */

enum {
  MAXFD	 = 63,	/* maximum user file descriptors */
  MAXPROC= 7,	/* maximum request pipes */
  MAXRLEN= 1024,/* maximum record length supported */
  MAXNAME= 128	/* maximum file name supported */
};

#if 0	/* old format - recoded in BMS file/network format */

/* #define ISRECNUM	/* emulate isrecnum */

typedef unsigned short ushort;

typedef struct {
#ifdef ISRECNUM
  long recnum;	/* isrecnum value */
#endif
  int len;	/* length parameter */
  int mode;	/* mode parameter */
  ushort p1;	/* first buffer length */
  ushort p2;	/* second buffer length */
  char fd;	/* file descriptor */
  char fxn;	/* request code */
} REQ;

typedef struct {
#ifdef ISRECNUM
  long recnum;	/* new isrecnum value */
#endif
  int res;	/* result code */
  int iserrno;
  ushort p1;	/* returned buffer length */
  char isstat1;
  char isstat2;
} RES;

#else
#include <ftype.h>

struct ISREQ {
  FLONG recnum;	/* isrecnum value */
  FSHORT len;	/* length parameter */
  FSHORT mode;	/* mode parameter */
  FSHORT p1;	/* first buffer length */
  FSHORT p2;	/* second buffer length */
  char fd;	/* file descriptor */
  char fxn;	/* request code */
};

struct ISRES {
  FLONG recnum;	/* new isrecnum value */
  FSHORT res;	/* result code */
  FSHORT iserrno;
  FSHORT p1;	/* returned buffer length */
  char isstat1;
  char isstat2;
};

#endif

enum isreqOp {
  ISBUILD,
  ISOPEN,
  ISCLOSE,
  ISADDINDEX,
  ISDELINDEX,
  ISSTART,
  ISREAD,
  ISWRITE,
  ISREWRITE,
  ISWRCURR,
  ISREWCURR,
  ISDELETE,
  ISDELCURR,
  ISUNIQUEID,
  ISINDEXINFO,
  ISAUDIT,
  ISERASE,
  ISLOCKOP,
  ISUNLOCK,
  ISRELEASE,
  ISRENAME,
  ISREWREC
};

int isreq(int fd,enum isreqOp,int l1,int l2,char *,char *,int mode,int len);