/*
	interface to node functions
*/
#include <string.h>
#include "node.def"	/* implementation defines */

typedef union node NODE;

void node_clear(BLOCK *);	/* initialize a node */

/* idx was returned by a previous call to find with the same record */
int node_insert(BLOCK *, int ,char *, int);

/* the number of bytes known to be the same in the user's key and
   the current record slot.  This is used for comparing keys, but
   can also be used to optimize node_copy().  If you are at all unsure
   whether it is still valid, use 0 instead for node_copy(). */

extern short node_dup;	/* current leading dup count */

/* extract a record: the last parameter is the number of bytes
   already in the buffer. returns the dup count of the extracted record */
int node_copy(BLOCK *,int ,char *, int, int);
void node_delete(BLOCK *,int ,int);

/* The caller guarrantees no key change for replace.
   The record is deleted on failure. */
int node_replace(BLOCK *,int ,char *, int);

/* return idx of record matching key. set node_dup to leading dup count.
   dir matters only with partial (i.e. "duplicate") keys */
int node_find(BLOCK *,char *,int , int/**/);

/* return number of chars identical in key and record */
short node_comp(BLOCK *,int ,char *,int);

/* return free bytes in node with n records */
#ifndef node_free
short node_free(BLOCK *,int);
#endif

#ifndef node_count
short node_count(BLOCK *);		/* return record count */
#endif

#ifndef node_size
short node_size(NODE *,int);	/* return size of record */
#endif

/* store block numbers in key records */

#ifndef node_stptr
void node_stptr(NODE *,int ,t_block *);
#endif

/* retrieve block numbers from key records */

#ifndef node_ldptr
void node_ldptr(NODE *,int ,t_block *);
#endif

/* move records between nodes.
   dir ==  1	end of source to start of destination
   dir == -1    start of source to end of destination */
short node_move(BLOCK *,BLOCK *,int ,int ,int);

void blkmovl(char *, const char *, int);
short blkcmp(const unsigned char *,const unsigned char *,int);
