static char what[] = "%W%";
/*
Date	:  12/13/91
Author 	:  Ed Bond
Function:  btfled.c - Edit file field tables.

Copyright (c) 1991 Business Management Systems, Inc.
*/

#include <string.h>
#include <stdio.h>
#include <errenv.h>
#include <ischkerr.h>
#include <keys.h>
#include <fs.h>
#include <fwindow.h>
#include <inq.h>
#include <fcb.h>
#include <memio.h>
#include <block.h>
#include <btflds.h>

#define KMAKE(e)	((e)|0x8000)
#define ESCKEY		KMAKE(1)
#define SCRNROWS 24
#define SCRNCOLS 80
#define FNAME	0
#define NKFLDS	1
#define NFLDS	2
#define FTYPE	3
#define FDESC	4
#define FLEN	5
#define FPOS	6

static struct btflds btflds;
static int nflds,nkflds;
static struct flddata {
  char type;
  short len,pos;
} flddata;

static MEMIO *mio = 0;
static FCB *fcblist[2];

static void fldedit(const char *);
static void loadar(),savear();
static int getfldtype(),putfldtype();
static const char *errname;

static FIELD TBL[] = {
  fldgen(NKFLDS,getnum,putnum,nkflds,0,0),
  fldgen(NFLDS,getnum,putnum,nflds,0,0),
  /* FCB1 */
  fldgen(FTYPE,getfldtype,putfldtype,flddata.type,0,0),
  fldgen(FLEN,getnum,putnum,flddata.len,"_",0),
  fldgen(FPOS,0,putnum,flddata.pos,"_",0)
};
#define FCB1 TBL+2
#define FCB1SZ 3

static void btflded(int argc,char **argv) {
  if (!mio) {
    dispscrn(errname = "btflded.scr",0);
    mio = memstart((PTR)&flddata,sizeof flddata);
    fcblist[0] = fcbbeg(FCB1,FCB1SZ,(ALTFKEY *)0,memio,(PTR)mio,
		 FCBFLG_TOP+FCBFLG_PHID);
  }
  while (--argc) {
    ++argv;
    fldedit(errname = *argv);
  }
}

static void fldedit(const char *name) {
  BTCB *bt = 0;
  int len = strlen(name);
  int pfkey;
  static KEYLIST helplist[] = {
      "Exit Help",0,
      "Escape (No save)",ESCKEY,
      "Reverse Page  F3",F3,
      "Forward Page  F4",F4,
      "Insert Line   F7",F4,
      "Delete Line   F8",F4,
      "Locate        F5",F5,
      "Quit          F10",F10,
      0,0 };

    putfield(FNAME,name,len);
    display();
    bt = btopen("",BTRDWR+BTDIROK,MAXREC);
    strcpy(bt->lbuf,name);
    bt->rlen = MAXREC;
    bt->klen = len+1;
    if (btas(bt,BTREADF + NOKEY))
      btflds.rlen = btflds.klen = 0;
    else
      ldflds(&btflds,bt->lbuf,bt->rlen);
    fcberaser(fcblist);
    loadar();
    putscrn(count(TBL),TBL);
    for (;;) {
      pfkey = fcbscrn(fcblist,count(TBL),TBL,(ALTFKEY *)0,helplist);
      if (pfkey == F10 || pfkey == RETURN) {
        savear();
        putscrn(2,TBL);	/* redisplay field counts */
        display();
        bt->rlen = stflds(&btflds,bt->lbuf);
        if (btas(bt,BTREPLACE + NOKEY)) {
	  bt->u.id.user = getuid();
	  bt->u.id.group = getgid();
	  bt->u.id.mode  = getperm(0666);
	  btas(bt,BTCREATE);
	}
	break;
      }
      if (pfkey == ESCKEY) break;
    }
    btas(bt,BTCLOSE);
}

static void loadar(void) {
  int i;
  for (i=0; i < btflds.rlen; ++i) {
    flddata.type = btflds.f[i].type;
    flddata.len = btflds.f[i].len;
    flddata.pos = btflds.f[i].pos;
    memio(FCBADD_AFTER,(PTR)0,(PTR)mio);
  }
  nflds = btflds.rlen;
  nkflds = btflds.klen;
}

static void savear(void ) {
  int i = 0;
  if (memio(FCBFIRST,(char *)0,(char *)mio))
  do {
    if (flddata.len != 0) {
      btflds.f[i].type = flddata.type;
      btflds.f[i].len = flddata.len;
      btflds.f[i].pos = flddata.pos;
      ++i;
    }
  } while (memio(FCBNEXT,(char *)0,(char *)mio));
  btflds.rlen = i;
  btflds.klen = nkflds;
}

static struct { char *mnem, type, *desc; } typelist[] = {
  "  ", 0,		"",
  "C ", BT_CHAR,	"Character",
  "D ", BT_DATE,	"Date",
  "X ", BT_BIN,		"Binary",
  "T ", BT_TIME,	"Time",
  "N0", BT_NUM,		"Number",
  "N1", BT_NUM+1,	"Number (1)",
  "N2", BT_NUM+2,	"Number (2)",
  "N3", BT_NUM+3,	"Number (3)",
  "N4", BT_NUM+4,	"Number (4)",
  "N5", BT_NUM+5,	"Number (5)",
  "N6", BT_NUM+6,	"Number (6)",
  "N7", BT_NUM+7,	"Number (7)",
  "N8", BT_NUM+8,	"Number (8)",
  "N9", BT_NUM+9,	"Number (9)"
};

static int getfldtype(int fld,char *data,const char *opt,int len) {
  int i;
  char old = *data, buf[2];
  getfield(fld,buf,sizeof buf);
  rpad(buf,sizeof buf);
  upper(buf,sizeof buf);
  for (i=0; i < count(typelist); ++i) {
    if (!strncmp(buf,typelist[i].mnem,sizeof buf)) {
      *data = typelist[i].type;
      break;
    }
  }
  if (i == count(typelist)) gethex(fld,data,opt,len);
  if (old != *data) putfldtype(fld,data,opt,len);
  return 0;
}

static int putfldtype(int fld,const char *data,const char *opt,int len) {
  int i;
  for (i=0; i < count(typelist); ++i) {
    if (*data == typelist[i].type) {
      putfield(fld,typelist[i].mnem,2);
      putfield(fld+1,typelist[i].desc,80);
      return 0;
    }
  }
  if (*data == 0) {
    putfield(fld,"",0);
    putfield(fld+1,"",0);
  }
  else {
    puthex(fld,data,opt,len);
    putfield(fld+1,"unknown",10);
  }
  return 0;
}

int main(int argc,char **argv) {
  int err = 0;
  bmsprog = argv[0];
  if (argc < 2) {
    printf("\nUsage: %s file [file]\n\n",bmsprog);
    return 1;
  }
  fsinit();
catch(err)
  btflded(argc,argv);
envend
  fsend();
  if (err) errdesc(errname ? errname : "",err);
  return 0;
}
