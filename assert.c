#include <stdio.h>
#include <assert.h>
#include <bterr.h>
#include "btree.h"

volatile void ASSERTFCN(const char *ex,const char *file,int line) {
  fprintf(stderr,"Assertion failure: %s line %d \"%s\"\n",file,line,ex);
  btpost(BTERINT);
}
