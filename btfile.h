/*
	File and directory handling module.
*/
	
int openfile(BTCB *,int);	/* open/stat file, check security */
void closefile(BTCB *);		/* close file */
t_block creatfile(BTCB *);	/* create new file */
t_block linkfile(BTCB *);	/* link existing file */
int delfile(BTCB *, t_block);	/* unlink file */
int btjoin(BTCB *);		/* join file */
int btunjoin(BTCB *);		/* unjoin file */

struct btredirect {
  long pid;		/* server process */
};
