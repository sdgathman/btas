/*
 * $Id$
 * Request format for cisam local server
 * $Log$
 * Revision 1.6  2003/03/18 21:38:34  stuart
 * Make isreq a public interface.
 *
 * Revision 1.5  2003/03/12 22:41:53  stuart
 * Create isreq encapsulated C-isam call interface.
 *
 * Revision 1.4  2002/03/20 20:07:14  stuart
 * Support ISMKDIR/ISRMDIR in isserve.
 * Include istrace in package.
 *
 * Revision 1.3  2001/02/28 23:09:27  stuart
 * new ops
 *
 * Revision 1.2  1997/04/30  19:31:24  stuart
 * add missing ops
 *
 * Revision 1.1  1994/02/13  20:38:25  stuart
 * Initial revision
 *
 */

#if 0	/* old format - recoded in BMS file/network format */

#define ISRECNUM	/* emulate isrecnum */

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
  unsigned char fd;	/* file descriptor */
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

int stkeydesc(const struct keydesc *k,char *buf);
void ldkeydesc(struct keydesc *k,const char *p);
int stdictinfo(const struct dictinfo *d,char *p);
void lddictinfo(struct dictinfo *d,const char *p);

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
  ISREWREC,
  ISSTARTN,
  ISDELREC,
  ISSETRANGE,
  ISADDFLDS,
  ISGETFLDS,
  ISREADREC,
  ISINDEXNAME,
  ISMKDIR
};

int isreq(int fd,int op,int *l1,int l2,char *,const char *,int mode,int len);

/** Return maximum result length given logical record length.
  * Possible results:
  * ISUNIQUE 4 byte id
  * ISREADREC rlen + 4 byte recnum
  * GETFLDS field table, max 2 * rlen
  * ISINDEXINFO dictinfo or keydesc, 52 (call it 64)
  * ISINDEXNAME index name, MAXKEYNAME = 32
  */
#define ISREQ_MAXRES(rlen) ((rlen < 32) ? 64 : rlen * 2)

#ifndef __cplusplus
enum {
  MAXFD	 = 63,	/* maximum user file descriptors */
  MAXPROC= 7,	/* maximum request pipes */
  MAXRLEN= 1024,/* maximum record length supported */
  MAXNAME= 128	/* maximum file name supported */
};
#endif
