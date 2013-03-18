/*  Experimental asynchronous write version of DEV.  Not in production.
  
    Copyright (C) 1985-2013 Business Management Systems, Inc

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include <unistd.h>
#include <errno.h>
#include <program.h>
#include "btdev.h"

PipeDEV::PipeDEV() {
  prog = 0;
  writerfd = -1;
  resfd = -1;
}

PipeDEV::~PipeDEV() {
  close();
}

int PipeDEV::open(const char *name,bool rdonly) {
  int rc = DEV::open(name,true);
  if (rc) return rc;
  prog = new Program;
  writerfd = prog->pipein(0);
  resfd = prog->pipeout(1);
  const char *argv[3];
  argv[0] = "btwriter";
  argv[1] = name;
  argv[2] = 0;
  rc = prog->run("btwriter",argv);
  return rc;
}

int PipeDEV::close() {
  ::close(writerfd);
  writerfd = -1;
  ::close(resfd);
  resfd = -1;
  delete prog;
  prog = 0;
}

int PipeDEV::write(t_block blk,const char *buf) {
  if (::write(writerfd,&blk,sizeof blk) != sizeof blk)
    return errno;
  if (::write(writerfd,buf,blksize) != blksize)
    return errno;
  return 0;
}

int PipeDEV::sync() {
  t_block zero = 0L;
  char buf[SECT_SIZE];
  gethdr(buf,sizeof buf);
  if (::write(writerfd,&zero,sizeof zero) != sizeof zero)
    return errno;
  if (::write(writerfd,buf,SECT_SIZE) != SECT_SIZE)
    return errno;
  return 0;
}

int PipeDEV::wait() {
  int rc;
  if (::read(resfd,&rc,sizeof rc) != sizeof rc)
    return errno;
  return rc;
}
