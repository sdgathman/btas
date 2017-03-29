/* types used by BTAS users and internal programs
   this file may need to be tuned for a particular system 

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

#ifndef BTMAGIC
#include <sys/types.h>
enum {
  MAXDEV    = 20,	/* max extents in a filesystem */
  BLKSIZE   = 1024,	/* default block size */
  MAXREC    = 502,	/* (BLKSIZE - sizeof(struct stem))/2 - sizeof(short) */
  MAXKEY    = 256	/* maximum key size */
};

typedef int32_t t_block;

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
  int32_t bcnt;		/* block count */
  int32_t rcnt;		/* record count */
  int32_t atime;	/* access time */
  int32_t ctime;	/* file create time */
  int32_t mtime;	/* modified time */
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
  int32_t space;	/* number of free blocks */
  int32_t mount_time;	/* time filesystem last mounted (unsigned?) */
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
  int32_t bufs, hsize;		/* buffer pool */
  int32_t searches, probes;	/* hashing statistics */
  int32_t lreads,preads,pwrites;	/* cache statistics */
  int32_t fbufs, fcnt;		/* btflush statistics */
  int32_t trans;			/* transaction count */
  int32_t sum2;			/* sum of squared I/O count */
  int32_t checkpoints;		/* number of checkpoints written */
  time_t uptime;			/* time server started */
  int32_t version;			/* server version */
  int32_t dirty;			/* dirty buffers in server */
  int32_t lwriteb;			/* total bytes in update requests */
};

#define BTMAGIC		0x0911
#define BTSAVE_MAGIC	0x0912

#endif
