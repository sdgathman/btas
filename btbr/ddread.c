static char what[] = "%W%";
#include <isamx.h>
#include <errenv.h>
#include <ischkerr.h>
#include <stdlib.h>
#include <string.h>
#include <ftype.h>
#include <btflds.h>
#include <block.h>
#include "fld.h"

struct dd {
  char fname[18];
  FSHORT line;
  FSHORT offset;
  char name[8];
  char type[1];
  char unused[1];
  FSHORT len;
  char mask[15];
  char desc[24];
};

FLD *ddread(filename,buf,rlen)
  char *filename,*buf;
  int rlen;
{
  int ddf = -1, i, rc, numflds = 0;
  struct dd dd;
  FLD *fld;
  char fname[sizeof dd.fname];
  char name[sizeof dd.name+1];
  char desc[sizeof dd.desc+1];

  stchar(filename,fname,sizeof fname);
  ddf = isopenx("DATA.DICT",READONLY,sizeof (struct dd));
  if (ddf != -1) {
    move(dd.fname,fname);
    stshort(0,dd.line);
    rc = chkread(ddf,dd,ISGTEQ,ISEOF);
    if (rc || isnequal(dd.fname,fname)) {
      isclose(ddf);
      ddf = -1;
    }
  }
  if (ddf == -1) {
    ddf = isopenx("/DATA.DICT",READONLY,sizeof (struct dd));
    if (ddf == -1) return 0;
  }

  move(dd.fname,fname);
  stshort(0,dd.line);
  rc = chkread(ddf,dd,ISGTEQ,ISEOF);
  while (!rc && isequal(dd.fname,fname)) {
    numflds++;
    rc = chkread(ddf,dd,ISNEXT,ISEOF);
  }
  if (!numflds) {
    isclose(ddf);
    return 0;
  }
  fld = malloc(numflds * sizeof *fld);
  if (!fld) return 0;
  move(dd.fname,fname);
  stshort(0,dd.line);
  rc = chkread(ddf,dd,ISGTEQ,ISEOF);
  for (i=0; !rc && i < numflds; ++i) {
    ldchar(dd.name,sizeof dd.name,name);
    ldchar(dd.desc,sizeof dd.desc,desc);
    fld[i].name = strdup(name);
    fld[i].data = buf+ldshort(dd.offset);
    fld[i].desc = strdup(desc);
    fld[i].len = ldshort(dd.len);
    fld[i].format = 0;
    switch (dd.type[0]) {
    case 'N': case 'n':
      if (fld[i].len < 1 || fld[i].len > 6) {
      /* if we have narrow or wide number make type BT_BIN */
	fld[i].type = BT_BIN;
	fld[i].dlen = fld[i].len * 2;
      }
      else {
	char mask[sizeof dd.mask+1];
	fld[i].type = BT_NUM;
	ldchar(dd.mask,sizeof dd.mask,mask);
	fld[i].format = strdup(mask);
	fld[i].dlen = strlen(mask);
      }
      break;
    case 'J': case 'j':
    case 'D': case 'd':
      fld[i].type = BT_DATE;
      fld[i].dlen = 8;
      break;
    case 'M': case 'm':
    case 'T': case 't':
      fld[i].type = BT_TIME;
      fld[i].dlen = 19;
      break;
    case 'C': case 'c':
      fld[i].type = BT_CHAR;
      fld[i].dlen = fld[i].len;
      break;
    default:
      /* unknown types get set to BT_BIN */
      fld[i].type = BT_BIN;
      fld[i].dlen = fld[i].len * 2;
      break;
    }
    fld[i].put = fld[i].type;
    fld[i].next = &fld[i+1];

    rc = chkread(ddf,dd,ISNEXT,ISEOF);
  }
  isclose(ddf);
  if (i == 0) {		/* if deleted before we loaded it */
    free(fld);
    return 0;
  }
  fld[i-1].next = 0;
  return fld;
}
