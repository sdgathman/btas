#include <stdio.h>
#include <stdlib.h>

int main(argc,argv)
  int argc; char **argv;
{
  int x;
  long i;
  long *ip = &i;
  while (--argc) {
    int j;
    i = atol(*++argv);
    printf("%ld = 0x",i);
    for (j=0;j<sizeof i;++j) {
      printf("%2.2X",(int)ip[j]);
    }
    printf(" = ");
    for (j=0;j<sizeof i;++j) {
      printf("%d,",(int)ip[j]);
    }
    printf("\n");
  }
  return 0;
}
