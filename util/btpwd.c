#include <stdio.h>
#include <btas.h>

main(int argc,char **argv) {
  int i;
  if (argc < 2) {
    puts(btgetcwd());
    return 0;
  }
  for (i = 1; i < argc; ++i) {
    if (btchdir(argv[i])) {
      fprintf(stderr,"Can't chdir to %s\n",argv[i]);
      return 1;
    }
    puts(btgetcwd());
  }
  return 0;
}
