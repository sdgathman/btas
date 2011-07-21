#include <ftype.h>

#define FNAMESIZE	18
struct dictrec {
  char file[FNAMESIZE];
  FSHORT seq;
  FSHORT pos;
  char name[8];
  char type[2];
  FSHORT len;
  char fmt[15];
  char desc[24];
};

/** Delete any local DATA.DICT records for tblname. */
int isdictdel(const char *tblname);
/** Add DATA.DICT records for tblname. */
int isdictadd(const char *tblname,const char *colname[],struct btflds *f);
