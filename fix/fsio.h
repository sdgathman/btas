#pragma interface

struct fsio {
  typedef long long t_off64;
  virtual int open(const char *name,int flag) = 0;
  virtual int read(int ext,char *buf,int size) = 0;
  virtual int write(int ext,const char *buf,int size) = 0;
  virtual t_off64 seek(int ext,t_off64 pos) = 0;
  enum { FS_RDONLY = 1, FS_BGND = 2, FS_SWAP = 4 };
  virtual ~fsio() { }
};

struct unixio: virtual fsio {
  unixio(int maxcnt);
  int open(const char *name,int flag);
  int read(int ext,char *buf,int size);
  int write(int ext,const char *buf,int size);
  t_off64 seek(int ext,t_off64 pos);
  ~unixio();
private:
  int dcnt, maxcnt;
  int *fd;
};
