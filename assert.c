#include <stdio.h>
#include <assert.h>
#include <bterr.h>
#include "btree.h"

#ifndef ASSERTFCN
#define ASSERTFCN _assert
#endif
extern "C"
volatile void ASSERTFCN(const char *ex,const char *file,int line) {
  fprintf(stderr,"Assertion failure: %s line %d \"%s\"\n",file,line,ex);
  btpost(BTERINT);
}
#if defined(linux)
extern "C" void __assert_fail (__const char *ex,
                                __const char *file,
                                unsigned int line,
                                __const char *function)
      {
  fprintf(stderr,"Assertion failure: %s %s:%d \"%s\"\n",file,function,line,ex);
  btpost(BTERINT);
}
#endif
