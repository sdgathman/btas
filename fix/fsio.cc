#pragma implementation
#include <unistd.h>
#include <fcntl.h>
#include "fsio.h"

unixio::unixio(int cnt) {
  dcnt = 0;
  fd = new int[maxcnt = cnt];
  for (int i = 0; i < maxcnt; ++i)
    fd[i] = -1;
}

unixio::~unixio() {
  for (int i = 0; i < dcnt; ++i)
    close(fd[i]);
  delete [] fd;
}

int unixio::open(const char *name,int flag) {
  if (dcnt >= maxcnt) return -1;
  int f = 0;
  if (name)
    f = ::open(name,(flag & FS_RDONLY) ? O_RDONLY : O_RDWR);
  if (f < 0) return f;
  fd[dcnt] = f;
  return dcnt++;
}

int unixio::read(int ext,char *buf,int sz) {
  return ::read(fd[ext],buf,sz);
}

int unixio::write(int ext,const char *buf,int sz) {
  return ::write(fd[ext],buf,sz);
}

long unixio::seek(int ext,long pos) {
  return ::lseek(fd[ext],pos,0);
}
