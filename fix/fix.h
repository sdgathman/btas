#ifndef FIX
#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#define FIX
#include "../btree.h"

enum { SECT_SIZE = 512 };

static inline int blk_dev(t_block b) { return (unsigned char)(b >> 24); }
static inline long blk_off(t_block b) { return b & 0xFFFFFFL; }
static inline t_block mk_blk(short i,long offset) { return (i<<24L)|offset; }
static inline long blk_pos(t_block b,unsigned s) {
  return (b - 1) * s + SECT_SIZE ;
}

struct fsio;
struct fstbl {
  /* const struct fs_class *_class; */
  char *buf;		/* I/O buffer */
  /* int fd[MAXDEV];	/* OS file descriptors for extent(s) */
  struct fsio *io;
  short nblks;		/* max number of blocks in I/O buffer */
  short cnt;		/* actual number of blocks in I/O buffer */
  /* sequential status */
  t_block base;		/* base block of current I/O buffer */
  short cur;		/* current block offset in I/O buffer */
  short curext;		/* current extent */
  bool seek;		/* true if seek required for next buffer */
  char flags;		/* flags passed in open */
  /* last block read */
  char *last_buf;
  t_block last_blk;
  /* superblock */
  union {
    struct btfs f;		/* superblock structure */
    char buf[SECT_SIZE];	/* sector buffer for superblock */
  } u;
  int (*typex)(const void *, t_block);
};

/* flags used by fsopen() */
enum { FS_RDONLY = 1, FS_BGND = 2 };

enum blk_type { BLKERR, BLKDEL = 1, BLKROOT = 2, BLKSTEM = 4, BLKLEAF = 8,
  /* enhanced block type flags apply to BLKROOT only */
      BLKCOMPAT =	0x0F,	/* mask for compatible types */
      BLKDIR	=	0x40,	/* block belongs to a directory */
      BLKDATA	=	0x80	/* block contains data (not index) */
};

#if defined(__GNUC__) || defined(__cplusplus)
static inline long lastblock(const struct fstbl *fs) { return fs->last_blk; }
#else
#define lastblock(fs) fs->last_blk
#endif

/* getblock.c */
struct fstbl *fsopen(const char *, int);
struct fstbl *fsopentbl(struct fstbl *,const char *);
struct fstbl *blkopen(struct fsio *,int flags);
struct fstbl *fsmopen(int);	/* open by mid */
union btree *getblock(struct fstbl *, t_block);
union btree *fsRndBlock(struct fstbl *, t_block);
union btree *fsSeqBlock(struct fstbl *);
void fsclose(struct fstbl *);
void fsclosetbl(struct fstbl *);
void blkclose(struct fstbl *);

/* delblock.c */
void fsclear(struct fstbl *);
void putblock(struct fstbl *);
void delblock(struct fstbl *);

/* blktype.c */
int buftypex(const void *, t_block);
int blktypex(struct fstbl *, t_block);
#define buftype(buf,blk)	(buftypex(buf,blk)&BLKCOMPAT)
#define blktype(fs,blk)		(blktypex(fs,blk)&BLKCOMPAT)

/* logdir.c */
void logroot(t_block, const struct btstat *);
void logdir(t_block, const char *, int, t_block);
void logrecs(t_block, int);
void logprint(t_block);

struct root_n {
  long root;
  struct dir_n *dir, *last;
  char *path;	/* non zero after first listing */
  struct btstat st;
  long bcnt, rcnt;	/* actual blocks & records that we count */
  short links;		/* actual links we see */
};

void doroot(struct root_n *);
#ifdef __cplusplus
}
#endif
#endif
