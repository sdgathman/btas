// route printf calls to the message() routine that is specialized
// for a specific environment

#include <stdio.h>
#include <stdarg.h>
#include "btserve.h"

int printf(const char *fmt,...) {
  char buf[1024];
  int rc;
  va_list ap;
  va_start(ap,fmt);
  rc = vsprintf(buf,fmt,ap);
  va_end(ap);
  message(buf);
  return rc;
}

int fprintf(FILE *f,const char *fmt,...) {
  char buf[1024];
  int rc;
  va_list ap;
  &f;
  va_start(ap,fmt);
  rc = vsprintf(buf,fmt,ap);
  va_end(ap);
  message(buf);
  return rc;
}
