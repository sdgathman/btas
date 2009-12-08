/*
 * $Log$
 * Revision 1.4  2002/11/06 02:55:24  stuart
 * Recognize and convert byteswapped image archives.  This only works when
 * alignments match.
 *
 * Revision 1.3  2001/02/28 22:53:45  stuart
 * virtual dtor
 *
 * Revision 1.2  2000/10/31  22:01:05  stuart
 * Make an object
 */

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
  virtual ~Logdir();
  struct root_n {
    long root;
    struct dir_n *dir, *last;
    char *path;		/* non zero after first listing */
    struct btstat st;
    long bcnt, rcnt;	/* actual blocks & records that we count */
    short links;	/* actual links we've seen */
  };
protected:
  virtual void doroot(root_n *);
private:
  root_n *addroot(long blk, const struct btstat *st);
  long printroot(root_n *r,int lev,const char *name);
  static void logchk(PTR p);
};
