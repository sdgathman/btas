#pragma interface

struct fsio {
  virtual int open(const char *name,int flag) = 0;
  virtual int read(int ext,char *buf,int size) = 0;
  virtual int write(int ext,const char *buf,int size) = 0;
  virtual long seek(int ext,long pos) = 0;
  enum { FS_RDONLY = 1, FS_BGND = 2 };
  virtual ~fsio() { }
};

struct unixio: virtual fsio {
  unixio(int maxcnt);
  int open(const char *name,int flag);
  int read(int ext,char *buf,int size);
  int write(int ext,const char *buf,int size);
  long seek(int ext,long pos);
  ~unixio();
private:
  int dcnt, maxcnt;
  int *fd;
};
