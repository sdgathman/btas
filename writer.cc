/*  Experimental asynchronous write version of DEV.  
  
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

   This is the writer process.  There is one for each mounted filesystem.
   After collecting a complete transaction, it writes a checkpoint, then
   begins updating the filesystem.  As each block is updated, it sets a flag
   in the shared memory buffer so that the reader process can discard the
   buffer if it needs to.
 */

class Writer {
  int pipefd;
  Array<long> bufs;
public:
  Writer();
  void write(BLOCK *);
  void sync();		// wait until writer finished
  ~Writer();
};

Writer::Writer() {
}

Writer::~Writer() { }

void Writer::write(BLOCK *bp) {
  bufs.add(bufindex(bp));
}

void writer(int fd) {
  // read null terminated list of buffers
  read(fd,list,n*sizeof list[0]);
}

void usage() {
  fputs("Usage:	btupdate osfile\n",stderr);
  exit(1);
}

int main(int argc,char **argv) {
  DEV ...   not finished
}
