#include <bttype.h>

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
