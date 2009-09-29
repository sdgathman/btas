/*
	btree operations all use the following structure.  The
	key/record is supplied and returned in 'lbuf'.  Additional
	interface routines may be used to copy logical records in and
	out of 'lbuf' if required.  Note that for secondary keys and
	variable length records, the data must be copied anyway.
*/
#ifndef BTASKEY
#include <port.h>
#include <string.h>

#ifdef __MSDOS__
#define BTASVECT	0x7f		/* server interrupt vector */
#define	geteuid()	0
#define	getegid()	0
#define	getuid()	0
#define	getgid()	0
#endif

#include "bttype.h"

typedef struct BTCB {
  long msgident;	/* message originator */
  t_block root; 	/* root node - identifies file */
  short mid;		/* mount id  - identifies file system */
  short flags;		/* status bits */
  union {
    struct btlevel cache;	/* speeds sequential operations */
    struct btperm id;		/* permissions for open/create */
  } u;
  short op;		/* operation code / result code */
  short klen;		/* key length for operation */
  short rlen;		/* record length for operation / max record length */
  char  lbuf[MAXREC];	/* logical record for operation */
} BTCB;

enum btop {
  BTREADGT,		/* read next higher key */
  BTREADGE,		/* read same or higher key */
  BTREADEQ,		/* read unique key */
  BTREADLE,		/* read same or lower key */
  BTREADLT,		/* read next lower key */
  BTREADF,		/* read first with same key */
  BTREADL,		/* read last with same key */
  BTREREAD,		/* read more bytes from current record */
			/* like unix read() */
  BTWRITE,		/* write new record - key must not exist */
  BTREWRITE,		/* write over existing record - key must be unique */
			/* like unix write() */
  BTREPLACE,		/* replace existing record - key must be unique */
  BTDELETE,		/* delete existing record - key must be unique */
  BTDELETEF,		/* delete first record with same key */
  BTDELETEL,		/* delete last record with same key */
  BTOPEN,		/* open file, return directory record */
  BTCREATE,		/* create file */
  BTSEEK,		/* change record position - used by BTREAD, BTREWRITE */
			/* like unix lseek() */
  BTLINK,		/* create link to existing file */
  BTTOUCH,		/* write file times, permissions */
  BTSTAT,		/* read file times, permissions, size, records */
  BTFLUSH,		/* flush file system buffers */
  BTMOUNT,		/* mount file system, open root */
  BTJOIN,		/* attach file system to directory */
  BTUNJOIN,		/* detach file system */
  BTCLOSE,		/* close file */
  BTUMOUNT,		/* close & unmount filesystem */
	/* used by rebuild programs, must be BTAS or super user */
  BTFREE,		/* delete block */
  BTUNLINK,		/* unlink file without adding to free list */
  BTZAP,		/* clear free list */
  BTRELEASE,		/* release all locked records */
	/* performance statistics for tuning */
  BTPSTAT,		/* server statistics */
  BTREGISTER		/* trap file operations */
};

extern char btopflags[32];	/* required modes for operations */

/* error classes.  Operation codes can be added to the following
   error classes.  By default, any error causes an errpost() with
   the BTAS error code.  Using one or more error classes causes errors
   belonging to those classes to be returned to the caller. */

enum {
  DUPKEY = 0x100,	/* allow DUPKEY errors */
  NOKEY = 0x200,	/* allow nokey(nofile) errors */
  LOCK = 0x400,		/* allow LOCK errors */
  BTERR = 0xf00,	/* allow any trapable error */

/* flag bits */

  BTEXCL =	0x2000,	/* exclusive open option */
  BT_OPEN =	0x0008,	/* btcb opened */
  BT_READ =	0x0004,	/* has read permission */
  BT_UPDATE =	0x0002,	/* has write permission */
  BT_EXEC =	0x0001,	/* has execute permission */
  BT_DIR =	0x1000,	/* directory file opened */
  BT_LOCK =	0x0FF0	/* lock count */
};

/*
	the primary interface.  opcode may be added to an error class(es).
	Errors not belonging to the specified error class (if any) will
	cause an errpost().
*/

int btas(BTCB *, int);

#define BTASKEY	0x5910911L	/* SYSV message queue */

/* The following routines are not strictly necessary, but will malloc()
   and free() a BTCB for you. */

BTCB *btopen(const char *, int, int);
int btpathopt(BTCB *, char *);
char *pathopt(char *, const char *);
int btopenf(BTCB *, const char *, int, int);
BTCB *btopendir(const char *, int);
int btclose(BTCB *);
int btkill(const char *);
int btclear(const char *);
int btlink(const char *, const char *);
int btslink(const char *, const char *);
int btrename(const char *, const char *);
int btmkdir(const char *,int), btrmdir(const char *);
int btstat(const char *, struct btstat *);
int btfstat(BTCB *, struct btstat *);
/* low level stat */
int btlstat(const BTCB *dir, struct btstat *,struct btlevel *);
int btfsstat(int drive,struct btfs *fs);	/* status of mounted fs */
int btchdir(const char *);
int btsync(void);		/* flush all filesystems */
char *btgetcwd(void);
BTCB *btgetdir(void);
extern BTCB *btasdir;
void btputdir(BTCB *);
int btlock(BTCB *, int);
void btunlock(BTCB *);

/* modes for btopen: one of these */
enum {
  BTRDONLY,	/* open for reading */
  BTWRONLY,	/* open for writing */
  BTRDWR,	/* open for both */
  BTNONE,	/* just open */
  /* add this to open directories */
  BTDIROK 	/* directory OK */
};
		/* add an error class (above) to return error codes */

/* a bttag is a place holder for an open file.  A bttag is much smaller
   than a BTCB and is therefore more convenient for some applications.
   a bttag must be transferred into and out of a BTCB before and after
   performing BTAS/X operations. */

struct bttag {
  long ident;
  t_block root;
  short mid, flags;
};

void btcb2tag(const BTCB *b, struct bttag *t);
void tag2btcb(BTCB *b, const struct bttag *t);

/* maximum record size for a given block size */
#define btmaxrec(blksize) (blksize/2-10)
/* BTCB size for a given maximum record size */
#define btsize(maxrec)	(maxrec + sizeof (BTCB) - MAXREC)

struct btff {
  BTCB *savdir;		/* saved directory */
  BTCB *b;		/* directory BTCB */
  const char *s;	/* filename pattern */
  char *dir;		/* directory path */
  int slen;		/* size of matching prefix */
};

int findfirst(const char *, struct btff *);
int findnext(struct btff *);
void finddone(struct btff *);
int match(const char *s, const char *pat);	/* match filename pattern */
typedef int (*btwalk_fn)(const char *path, const struct btstat *);
typedef int (*btwalk_err)(const char *path, int code);
int btwalk(const char *path, btwalk_fn);
int btwalkx(const char *path, btwalk_fn,btwalk_err);
int bt_klen(const char *), bt_rlen(const char *,int);

#endif
