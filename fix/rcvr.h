#pragma interface
extern "C" {
#include <btas.h>
}
/** Info about one file to be recovered from blocks belonging
    to a root node.  
    
    If the source root node is zero, recovery is "in place" and our
    ctor disconnects the original file and creates a new empty file.

    Otherwise, records extracted from input blocks will be added to
    the existing file.

    For "in place", the caller will usually delete blocks from the original
    file as they are processed.

    Our dtor restores the original file if no blocks have been
    processed, otherwise the new file has the recovered data.

    If we were interrupted, some data may still be under the old root
    and may be recovered by appending from the old root to the already
    recovered data.
 */

class rcvr {
  char *name;		/* pathname of file being recovered */
  t_block root;		/* old root id */
  long blks;		/* blocks processed */
  long recs;		/* records processed */
  long dups;		/* duplicates */
  struct bttag tag;	/* BT_DIR is turned off */
  bool undo;
  /** add one record to a file */
  void write(BTCB *b);
  t_block dirstat(BTCB *dirf,const char *name,bool interactive = false);
public:
  /** interface to process directory records
   */
  struct linkdir {
    virtual void link(rcvr *,const char *buf,int len,t_block root) = 0;
  };
  bool link(const char *buf,int len,t_block root);
  void touch(const struct btstat *st);
  bool test() const { return !(tag.flags & BT_UPDATE); }
  rcvr();
  rcvr(const char *name,long rt,bool test=false,bool interactive=true);
  void setName(const char *n,long rt,bool test=false,bool interactive=false);
  /** Add records from a raw block to this file.  */
  int donode(union node *n,int maxrec,void *blkend);
  /** Enable directory entry prcoessing for this file */
  void setDir(linkdir *dir);
  /** Delete a raw block. */
  void free(t_block blk);
  static void summaryHeader();
  void summary() const;
  short mid() const { return tag.mid; }
  /** Return old rootid. */
  t_block rootid() const { return root; }
  /** Return new rootid. */
  t_block newroot() const { return tag.root; }
  const char *path() const { return name; }
  ~rcvr();
private:
  linkdir *dir;
  struct btstat st;
};
