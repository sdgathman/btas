/*
 * $Id$
 * Request format for cisam local server
 * $Log$
 * Revision 1.8  2003/07/29 16:46:35  stuart
 * isfdlimit entry point.  Always use ischkfd.
 *
 * Revision 1.7  2003/04/05 04:40:59  stuart
 * Sanity check keydesc.  Initialize kp->k.k_len in isaddindex().
 *
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
  ISMKDIR,
  ISCLEANUP	/* retry erases of open files */
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

/** Sets file descriptor limit.  Maximum file desciptor allocated will
  * be one less than the limit.  Used to ensure fds fit into 
  * the integer type reserved for them.  For example, isserve.c
  * uses an 8 bit field for file descriptor, and calls isfdlimit(255).
  * Return the previous setting, or 0 for no limit (other than memory).
  */
int isfdlimit(int maxfds);

#ifndef __cplusplus
enum {
  MAXFD	 = 63,	/* maximum user file descriptors (NOT USED) */
  MAXPROC= 7,	/* maximum request pipes (NOT USED) */
  MAXRLEN= 1024,/* maximum record length supported */
  MAXNAME= 128	/* maximum file name supported */
};
#endif
