#include "fs.h"
#include <Ftype.h>

#if 1
class s1_block {
  char str[4];
public:
  operator long() const { return ldlong(str); }
  void operator=(long v) { stlong(v,str); }
};

class s1_word {
  char str[2];
public:
  operator int() const { return ldshort(str); }
  void operator=(int v) { stshort(v,str); }
};
#else
typedef FNUM<4> s1_block;
typedef FNUM<2> s1_word;
#endif

struct s1btree {
  s1_block root;		// self or blocks + 0x80000000L if root
  union {
    struct {
      s1_block size;		// number of records
      s1_word lock,type;
    } r; 		// if root < 0
    struct {
      s1_block lbro,rbro;
    } s;		// if root > 0
  };
  s1_word count;
  union {
    s1_block son;
    struct {
      s1_word klen,rlen;
    } info;		// if son < 0
  };
  char data[256-18];
};

struct s1dir {
  unsigned char name[18];
  s1_word ver;
  s1_block date,time;
  s1_block root;
  s1_word klen,rlen;
  s1_block priv,term;
  s1_word user,acct;
  char who;
  char type;
  char res;
  char flag;
};

struct s1FS: btFS {
  struct s1fs {
    s1_word lock;	// volume lock / internal error count
    s1_word blk;	// block size in 256 byte sectors
    ECHAR<8> ver;	// FILE NAME verification
    s1_block spa;	// free nodes
    FJUL date;		// date of last save
    s1_block free;	// free block chain
    s1_block root;	// root of main directory
    s1_word klen,rlen;	// klen,rlen of main directory
    s1_word cnt;	// number of extents
    struct {
      s1_block eod;	// next free sector
      s1_block eof;	// total useable sectors
      s1_block orn;	// dataset origin (S/1 only)
      s1_word  vde;	// DDB address (S/1 only)
    } ds[125];		// extent table
  };
  s1FS(const char *name,int flag = FS_RDONLY);
  s1btree *get(t_block blk = 0);
  t_block lastblk() const;
  t_block xblk(t_block) const;
  t_block oblk(t_block) const;
  int type(t_block b) { return btFS::type(oblk(b)); }
  int typex(void *,t_block);
  ~s1FS();
  virtual String xname(const String &);	// translate EDX to os name
  short rlen, klen;	// record size of root dir
private:
  int fd;	// BTAS/1 control dataset
  short blksize;	// block size in 256 byte sectors
  static int fstypex(const void *,t_block);
  struct cache *c;
};
