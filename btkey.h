/*
	Intermediate level key lookup routines.  See btkey.c for
	a more detailed explanation.
*/

BLOCK *uniquekey(BTCB *);	/* find unique key */
BLOCK *btfirst(BTCB *);		/* find first matching key */
BLOCK *btlast(BTCB *);		/* find last matching key */
BLOCK *verify(BTCB *);		/* verify cached location */
int addrec(BTCB *);		/* add user record */
void delrec(BTCB *, BLOCK *);	/* delete user record */
