/* interface between btbuf.c and {no}hash.c */

/* hash.c || nohash.c */
int begbuf(char *, int, int);
void endbuf(void);
extern int curcnt;	/* needed by btnew, a kludge */

/* btbuf.c */
int writebuf(BLOCK *);
