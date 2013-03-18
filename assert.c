/* $Log$
    Convert assert macro to longjmp based exceptions used by btserve.
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
