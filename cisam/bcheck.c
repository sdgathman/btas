#include <stdio.h>
#include <isamx.h>

static void iserror(const char *desc) {
  fprintf(stderr,"%s: C-isam error %d\n",desc,iserrno);
}

int main(int argc,char **argv) {
  int i;
  if (argc < 2) {
    fputs("$Id$\n\
Usage:	bcheck mastfile [mastfile ...]\n\
	rebuild indexes specified in mastfile.idx\n",stderr);
    return 1;
  }
  for (i = 1; i < argc; ++i) {
    int fd = isopen(argv[i],ISINOUT+ISEXCLLOCK);
    int j;
    struct dictinfo d;
    if (fd == -1) {
      iserror(argv[i]);
      continue;
    }
    if (isindexinfo(fd,(struct keydesc *)&d,0)) {
      iserror(argv[i]);
      continue;
    }
    for (j = 2; j <= d.di_nkeys; ++j) {
      struct keydesc k;
      if (isindexinfo(fd,&k,j)) {
	iserror(argv[i]);
	continue;
      }
      fprintf(stderr,"%s:	rebuilding index #%d\n",argv[i],j);
      if (isfixindex(fd,&k)) {
	iserror(argv[i]);
	continue;
      }
      if (isrecnum != 0L)
	fprintf(stderr,"\t%ld records deleted because of DUPKEYs\n",isrecnum);
    }
    fprintf(stderr,"%s:	done\n",argv[i]);
    isclose(fd);
  }
}
