extern "C" {
#include "btserve.h"
}
#include "node.h"
#include "btdev.h"

enum { MAXFLUSH = 60 };

/* btbuf.c */
int btroot(t_block, short);	/* declare current root */
BLOCK *btbuf(t_block);		/* read block */
BLOCK *btread(t_block);	/* read block no root check */
BLOCK *btnew(short);		/* allocate block */
extern int newcnt;				/* counts new/deleted blocks */
extern long curtime;				/* BTAS time */
void btfree(BLOCK *);		/* free block */
int btumount(short);		/* unmount fs */
int btflush(void);		/* flush all buffers */
int writebuf(BLOCK *);
void btspace(void);		/* check free space */
void btdumpbuf(const BLOCK *);

extern class BufferPool *bufpool;
extern const char version[];

#ifdef __MSDOS__
#define btget(i) bufpool->get(i,__FILE__,__LINE__)
#else
#define btget(i) bufpool->get(i,what,__LINE__)
#endif
