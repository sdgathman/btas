#pragma interface
#include "node.h"
//#include "btdev.h"
#include "hash.h"

class BlockCache: public BufferPool {
  t_block root;			/* current root node */
  class FDEV *dev;		/* current file system */
  char *pool;			/* buffer storage */
  int poolsize;			/* number of buffers */
  unsigned nsize;		/* sizeof a buffer */
  BLOCK *getblock(int i) { return (BLOCK *)(pool + i * nsize); }
public:
  bool safe_eof;
  const int maxrec;
  BlockCache(int bsize,unsigned cachesize);
  void btcheck();		/* write pre-image checkpoint if required */
  void btspace(int needed);	/* check free space */
  int flush();		/* flush all buffers */
  int btroot(t_block root, short mid);	/* declare current root */
  BLOCK *btbuf(t_block blk);		/* read block */
  BLOCK *btread(t_block blk);		/* read block no root check */
  int writebuf(BLOCK *);		/* write block */
  BLOCK *btnew(short flags);		/* allocate block */
  void btfree(BLOCK *);			/* free block */
  short mount(const char *);		/* mount fs, return mid */
  int unmount(short mid);		/* unmount fs */
  static void dumpbuf(const BLOCK *);
  int newcnt;				/* counts new/deleted blocks */
  ~BlockCache();
};

extern const char version[];
extern const int version_num;

#ifdef __MSDOS__
#define btget(i) bufpool->get(i,__FILE__,__LINE__)
#else
#define btget(i) bufpool->get(i,what,__LINE__)
#endif
