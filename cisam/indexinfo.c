static char what[] = "%W%";
#include <stdio.h>
#include <stdlib.h>
#include <isamx.h>
#include <errenv.h>
#include <ischkerr.h>

static void prtidx(fd,idx)
  int fd,idx;
{
  struct keydesc k;
  int i;
  printf("Index #%d ",idx);
  chk(isindexinfo(fd,(struct keydesc *)&k,idx),0);
  printf("len=%-2d  %d part%s",k.k_len,k.k_nparts,(k.k_nparts==1 ? ": ":"s:"));
  for (i=0; i < k.k_nparts ; i++)
    printf(" %3d-%-2d",k.k_part[i].kp_start,k.k_part[i].kp_leng);
  putchar('\n');
}

int main(argc,argv)
  int argc;
  char *argv[];
{
  int err = 0, fd = -1, idx = 0;
  struct dictinfo d;

  bmsprog = argv[0];
  argc--; argv++;
  if (argc <= 0) {
    fprintf(stderr,"Usage: %s filename",bmsprog);
    return 1;
  }
catch(err)
  printf("File: %s ",argv[0]);
  chkopenx(fd,argv[0],ISINPUT,512);
  chk(isindexinfo(fd,(struct keydesc *)&d,0),0);
  printf("%d keys  rlen=%d  %ld records\n",
	  d.di_nkeys,d.di_recsize,d.di_nrecords);
  if (idx) prtidx(fd,idx);
  else while (++idx <= d.di_nkeys) prtidx(fd,idx);
envend
  iscloseall();
  if (err) errdesc("",err);
  return 0;
}
