/*
	dd.h - DATA.DICT structure
*/

#include <ftype.h>

struct ddst {
  char filename[18];
  FSHORT line;
  FSHORT offset;
  char fldname[8];
  char type[2];
  FSHORT len;
  char mask[15];
  char desc[24];
};
