/* types used by BTAS users and internal programs
   this file may need to be tuned for a particular system */

#ifndef BTMAGIC
enum {
  MAXDEV    = 20,	/* max extents in a filesystem */
  BLKSIZE   = 1024,	/* default block size */
  MAXREC    = 502,	/* (BLKSIZE - sizeof(struct stem))/2 - sizeof(short) */
  MAXKEY    = 256	/* maximum key size */
};

typedef long t_block;

struct btperm {
  short user,group;	/* file owner, group */
  short mode;		/* file type, permissions */
};

struct btlevel {	/* physical record/file address */
  t_block node;
  short slot;
};

/* status record returned by BTSTAT operation */

struct btstat {
  long bcnt;		/* block count */
  long rcnt;		/* record count */
  long atime;		/* access time */
  long ctime;		/* file create time */
  long mtime;		/* modified time */
  short links;		/* link count */
  short opens;		/* open count */
  struct btperm id;
#ifdef SAFELINK
  short level;		/* minimum distance from root directory */
#endif
};

/* filesystem header - must fit in disk sector */

struct btfhdr {
  t_block free;	/* next free list block */
  t_block droot;	/* current deleted root */
  char server;		/* server index */
  char rootext;		/* extent index of root file */
  unsigned short root;	/* sector offset of root file of filesystem */
  long space;		/* number of free blocks */
  long mount_time;	/* time filesystem last mounted (unsigned?) */
  unsigned char chkseq;	/* chkpoint sequence number */
  unsigned char errcnt;	/* filesystem errors detected since rebuild */
  unsigned short blksize;	/* block size for this device */
  short mcnt;		/* number of active opens */
  short dcnt;		/* number of devices */
  short magic;		/* BTAS/2 magic number */
  char baseid;		/* index into extent table of mounted filesystem */
  unsigned char flag;	/* chkpoint block count or ~0 for dirty */
};

struct btdev {
  t_block eod;	/* number of data blocks used */
  t_block eof;	/* maximum data blocks or 0 if expandable */
  char name[16];	/* OS pathname from directory of primary */
};

struct btfs {
  struct btfhdr hdr;
  struct btdev dtbl[MAXDEV];
};

/* record returned by BTPSTAT operation */

struct btpstat {		/* performance statistics */
  int bufs, hsize;		/* buffer pool */
  long searches, probes;	/* hashing statistics */
  long lreads,preads,pwrites;	/* cache statistics */
  long fbufs, fcnt;		/* btflush statistics */
  long trans;			/* transaction count */
  long sum2;			/* sum of squared I/O count */
  long checkpoints;		/* number of checkpoints written */
  long uptime;			/* time server started */
  long version;			/* server version */
  int dirty;			/* dirty buffers in server */
  long lwriteb;			/* total bytes in update requests */
};

#define BTMAGIC		0x0911
#define BTSAVE_MAGIC	0x0912

#endif
