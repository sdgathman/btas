#include "btree.h"

#define SECT_SIZE	512	/* OS sector size (min raw I/O size) */

extern int btbegin(/**/ int ,unsigned /**/);	/* initialize */
extern short btmount(/**/ char * /**/);		/* mount fs */
extern int btroot(/**/ t_block, int /**/);	/* declare current root */
extern void btget(/**/ int /**/);		/* reserve buffers */
extern BLOCK *btbuf(/**/ t_block /**/);		/* read block */
extern BLOCK *btread(/**/ t_block /**/);	/* read block no root check */
extern BLOCK *btnew(/**/ int /**/);		/* allocate block */
extern BLOCK *getbuf(/**/ t_block, int /**/);	/* find block, don't read */
extern int newcnt;				/* counts new/deleted blocks */
extern int safe_eof;				/* safe btspace() */
extern long curtime;				/* BTAS time */
extern void btfree(/**/ BLOCK * /**/);		/* free block */
extern void swapbuf(/**/ BLOCK *, BLOCK * /**/);/* swap block ids */
extern int btumount(/**/ int /**/);		/* unmount fs */
extern int btflush(/**/ void /**/);		/* flush all buffers */
extern void btend(/**/ void /**/);		/* shutdown */

extern struct btfhdr devtbl[MAXDEV];		/* mid table */
extern int gethdr(/**/ const struct btfhdr *, char *, int /**/);
extern void btspace(/**/ void /**/);		/* check free space */

extern struct btpstat stat;
