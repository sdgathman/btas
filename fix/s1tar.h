#include "s1fs.h"
#include <String.h>

class s1tar {
  String curdir;
  int fd;
  s1FS *fs;
  void seqout(void *,int);
  void chdir(const char *dir);
  void eof(int rlen);
public:
  enum { ECOMMA = 0x6B };
  s1tar(int f,s1FS *);
  long addfile(t_block root,const char *path);
  ~s1tar();
};
