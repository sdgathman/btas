#include "btree.h"

#define SECT_SIZE	512	/* OS sector size (min raw I/O size) */
#define MAXFLUSH	60	/* maximum modified blocks allowed */

/* btbuf.c */
int btbegin(int ,unsigned);	/* initialize */
short btmount(const char *);	/* mount fs */
int btroot(t_block, short);	/* declare current root */
BLOCK *btbuf(t_block);		/* read block */
BLOCK *btread(t_block);	/* read block no root check */
BLOCK *btnew(short);		/* allocate block */
extern int newcnt;				/* counts new/deleted blocks */
extern int safe_eof;				/* safe btspace() */
extern long curtime;				/* BTAS time */
void btfree(BLOCK *);		/* free block */
int btumount(short);		/* unmount fs */
int btflush(void);		/* flush all buffers */
void btend(void);		/* shutdown */

extern struct btfhdr devtbl[MAXDEV];		/* mid table */
int gethdr(const struct btfhdr *, char *, int);
void btspace(void);		/* check free space */

extern struct btpstat serverstats;

/* hash.c || nohash.c */
BLOCK *getbuf(t_block, short);	/* find block, don't read */
void swapbuf(BLOCK *, BLOCK *);/* swap block ids */
void btget(int);		/* reserve buffers */
void btget_deb(int,const char *,int);
#define btget(i) btget_deb(i,what,__LINE__)
