static char what[] = "@(#)fcbtodd.c	1.1";
/*
Date:      07/19/91
Author:	   Ed Bond
Function:  fcbtodd.c - Convert ".fcb" file to DATA.DICT file.
Copyright (c) 1991 Business Management Systems, Inc.
*/

#include <stdio.h>
#include <isamx.h>
#include <errenv.h>
#include <ischkerr.h>
#include <btflds.h>
#include "fld.h"
#include "dd.h"

static int ddf = -1;
static int fcbtodd(char *);

int main(argc,argv)
  int argc;
  char *argv[];
{
  int er;

  bmsprog = argv[0];
envelope
  chkopenx(ddf,"DATA.DICT",UPDATE,sizeof (struct ddst));
  while (--argc) fcbtodd(*++argv);
  errno = 0;
envend
  er = errno;
  iscloseall();
  if (er) errdesc("",er);
  return 0;
}

static int fcbtodd(name)
  char *name;
{
  register struct fld *flist = 0, *fp;
  register int i;
  struct ddst dd;
  char buf[MAXREC];

  flist = fcbread(name,buf,sizeof buf);
  if (!flist) {
    printf("%s.fcb: Can't open\n",name);
    return 1;
  }
  stchar(name,dd.filename,sizeof dd.filename);

  for (i = 0, fp = flist; fp; fp = fp->next, i++) {
    stshort(i,dd.line);					/* set line number */
    stshort((short)(fp->data-buf),dd.offset);		/* compute offset */
    stchar(fp->name,dd.fldname,sizeof dd.fldname);	/* copy field name */

    /* compute field type */
    if (fp->put&BT_NUM) dd.type[0] = 'N';
    switch (fp->put) {
      case BT_CHAR: dd.type[0] = 'C';
		    break;
      case BT_DATE: if (fp->len == 4) dd.type[0] = 'J';
                    else dd.type[0] = 'D';
		    break;
      case BT_BIN:  dd.type[0] = 'X';
		    break;
    }
    dd.type[1] = ' ';
    stshort(fp->len,dd.len);				/* copy length */
    stchar(fp->format,dd.mask,sizeof dd.mask);		/* copy mask */
    stchar(fp->desc,dd.desc,sizeof dd.desc);		/* copy description */
    if (chkwrite(ddf,dd,ISDUP)) chkrewrite(ddf,dd,ISDUP);
  }
  if (flist) fcbfree(flist);
  return 0;
}
