/* This is the writer process.  There is one for each mounted filesystem.
 * After collecting a complete transaction, it writes a checkpoint, then
 * begins updating the filesystem.  As each block is updated, it sets a flag
 * in the shared memory buffer so that the reader process can discard the
 * buffer if it needs to.
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
  DEV 
}
