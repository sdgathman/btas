// $Log$

#pragma interface
#include <bttype.h>

/** Model btas root nodes and directories in memory for diagnostics 
	and recovery.
    @author Stuart D. Gathman
    Copyright (C) 2000 Business Management Systems, Inc.
 */
class Logdir {
  struct tree *t;
  class Obstack &h;
  long lostcnt;
  char *path;
public:
  Logdir();
  /** Add a root node with attributes to model.  */
  void logroot(t_block root, const struct btstat *attr);
  /** Add a directory record to model. */
  void logdir(t_block root, const char *rec, int rlen, t_block link);
  /** Increment the record count for a root node. */
  void logrecs(t_block root, int recs);
  /** Print the directory tree starting with a given root. */
  void logprint(t_block root);
protected:
  struct root_n {
    long root;
    struct dir_n *dir, *last;
    char *path;		/* non zero after first listing */
    struct btstat st;
    long bcnt, rcnt;	/* actual blocks & records that we count */
    short links;	/* actual links we've seen */
  };
  virtual void doroot(root_n *);
  ~Logdir();
private:
  root_n *addroot(long blk, const struct btstat *st);
  long printroot(root_n *r,int lev,const char *name);
  static void logchk(PTR p);
};
