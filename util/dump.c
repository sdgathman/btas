/*
	Dump records using fcb from directory record
 *
 * $Log$
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <btflds.h>
#include <ftype.h>
#include <money.h>
#include <date.h>
#include <time.h>
#include "btutil.h"
#include "ebcdic.h"

int getdata(const char *, char *, struct btfrec *, int);
void dumprec(char *, int, struct btfrec *);
void hexrec(const char *, int, int);
char *hexs2bin(const char *,char *,int);

#define ERR ' '		/* 0177, translate untranslatable chars to this */

static char hextbl[] = "0123456789ABCDEF";

/* convert hex string to binary, len is max length to convert */
/* if len is negative then buffer is not padded with trailing zero bytes */
char *hexs2bin(const char *s,char *buf,int len) {
  int pad = 1;
  if (len < 0) { len = 0-len; pad = 0; }
  for (;len && *s;--len) {
    const char *p = strchr(hextbl,*s++);
    if (!p) break;
    *buf = (p - hextbl);
    if (!*s || (p = strchr(hextbl,*s++)) == 0) {
      ++buf;
      break;
    }
    *buf = (*buf<<4) + (p - hextbl);
    ++buf;
  }
  if (pad) while (len--) *buf++ = 0;
  return buf;
}

void dumprec(char *buf,int blen,struct btfrec *fcb) {
  int pos = 0;		/* current print column */
  while (fcb->type && blen > 0) {
    int len = 0, i;
    switch (fcb->type) {
    case BT_CHAR:
      while (*buf && len < fcb->len && blen > 0) {
	i = *buf++; --blen;
	while (i < ' ') {
	  (void)putchar(' '); ++len;
	  ++i;
	}
	(void)putchar(i); ++len;
      }
      if (len < fcb->len) {
	++buf;	/* skip null terminator */
	--blen;
	while (len < fcb->len) {
	  (void)putchar(' '); ++len;
	}
      }
      pos += len;
      blen += len;
      break;
    case BT_DATE:
      {
	struct mmddyy mdy;
	if (fcb->len < 4) {
	  DATE d = ldD(buf);
	  if (d == zeroD) printf("%8c",'0');
	  else {
	    mdy = doc_to_mdy(d);
	    (void)printf("%2d/%02d/%02d",mdy.mm,mdy.dd,mdy.yy);
	  }
	  pos += 8;
	}
	else {
	  JULDAY d = ldJ(buf);
	  if (d == zeroJ) printf("%10c",'0');
	  else {
	    julmdy(d,&mdy);
	    (void)printf("%2d/%02d/%d",mdy.mm,mdy.dd,mdy.yy);
	  }
	  pos += 10;
	}
	buf += fcb->len;
      }
      break;
    case BT_TIME:
      {
	long t = ldlong(buf);
	struct tm *p = localtime(&t);
	printf("%2d/%02d/%d %2d:%02d:%02d",p->tm_mon+1,p->tm_mday,
	       p->tm_year+1900,p->tm_hour,p->tm_min,p->tm_sec);
	pos += 17;
        buf += fcb->len;
      }
      break;
    default:		/* num types with precision less than 6 */
      if (fcb->type >= BT_NUM && fcb->type-BT_NUM < 6 && fcb->len <= 6) {
	MONEY mny;
	static char stbl[8] = { 0, 4, 6, 9, 11, 13, 16, 0 };
	char mask[20];
	char tmp[20];
	len = fcb->len;
	(void)memcpy(tmp,buf,len);
	while (blen < len) tmp[blen++] = 0;
	mny = ldnum(tmp,len);
	buf += len;
	len = stbl[fcb->len & 7] + (fcb->type > BT_NUM);
	(void)memset(mask,'Z',len);
	mask[len-1] = '#';
	mask[len] = 0;
	mask[0] = '-';
	pos += len;
	if (fcb->type > BT_NUM)
	  mask[len - fcb->type + BT_NUM - 1] = '.';
	(void)pic(&mny,mask,tmp);
	tmp[len] = 0;
	(void)fputs(tmp,stdout);
	break;
      }
    /* hex dump */
      while (len < fcb->len && len < blen) {
	int i = *buf++ & 0xff;
	putchar(hextbl[i>>4]);
	putchar(hextbl[i&15]);
	++len;
      }
      pos += len * 2;
    }
    blen -= fcb->len;
    (void)putchar(' ');
    ++fcb;
  }
  (void)putchar('\n');
}

void hexrec(const char *buf,int len,int flag) {
  register int i,j;
  for (i = 0; i < len; i += 16) {
    for (j = i; j-i < 16 && j < len; ++j) {
      putchar(hextbl[buf[j]>>4&15]);
      putchar(hextbl[buf[j]&15]);
      if (j & 1) putchar(' ');
    }
    for (j -= i; j < 16; ++j) {
      putchar(' ');
      putchar(' ');
      if (j & 1) putchar(' ');
    }
    putchar('|');
    for (j = i; j-i < 16 && j < len; ++j) {
      char c = buf[j];
      if (flag == 1) c = ebcdic(c);
      if (isascii(c) && isprint(c))
	putchar(c);
      else
	putchar('.');
    }
    puts("|");
  }
  putchar('\n');
}

int dump() {
  char *s = readtext("Filename to dump: ");
  BTCB *dtf;
  struct btflds fcb;
  char xopt = 0, ropt = 0, eopt = 0;
  int rc;
  if (!s) return 1;
  dtf = btopen(s,BTRDONLY+4,MAXREC);
  while (*switch_char) {
    switch (*switch_char++) {
    case 'r':
      ropt = 1;		/* dump in reverse */
      break;
    case 'h':
      xopt = 2;		/* dump two column hex records */
      break;
    case 'e':
      eopt = 1;		/* EBCDIC translation */
      xopt = 2;		/* dump two column hex records */
      break;
    case 'x':		/* dump hex records */
      xopt = 1;
    }
  }
  if (!dtf) {
    (void)printf("%s: not found\n",s);
    return 0;
  }
  if (dtf->flags & BT_DIR) {
    int i;
    static char dirf[] = { 0, 1, BT_CHAR, 24 };
    (void)ldflds(&fcb,dirf,sizeof dirf);
    for (i = 1; i < MAXFLDS - 1; ++i) {
      fcb.f[i].type = BT_BIN;
      fcb.f[i].len  = 1;
      fcb.f[i].pos  = fcb.f[i-1].pos + fcb.f[i-1].len;
    }
    fcb.f[i].type = 0;
    fcb.f[i].len  = 0;
    fcb.f[i].pos  = fcb.f[i-1].pos + fcb.f[i-1].len;
  }
  else if (xopt == 0)
    (void)ldflds(&fcb,dtf->lbuf,dtf->rlen);
  else {
    fcb.klen = fcb.rlen = 1;
    fcb.f[0].type = BT_BIN;
    fcb.f[0].len = MAXREC;
    fcb.f[0].pos = 0;
    fcb.f[1].type = 0;
    fcb.f[1].len  = 0;
    fcb.f[1].pos  = MAXREC;
  }
  dtf->klen = getdata((char *)0,dtf->lbuf,fcb.f,fcb.klen); /* key data */
  dtf->rlen = MAXREC;
  if (ropt)
    rc = btas(dtf,BTREADLE+NOKEY);
  else
    rc = btas(dtf,BTREADGE+NOKEY);
  while (rc == 0 && cancel == 0) {
    if (xopt == 2)
      hexrec(dtf->lbuf,dtf->rlen,eopt);
    else
      dumprec(dtf->lbuf,dtf->rlen,fcb.f);
    dtf->klen = (dtf->rlen < MAXKEY) ? dtf->rlen : MAXKEY;
    dtf->rlen = MAXREC;
    if (ropt)
      rc = btas(dtf,BTREADLT+NOKEY);
    else
      rc = btas(dtf,BTREADGT+NOKEY);
  }
  (void)btclose(dtf);
  return 0;
}

int getdata(const char *s,char *obuf,struct btfrec *f,int n) {
  register char *buf = obuf;
  char prompt[40];
  char *p,*mesg = 0,*ftype;
  MONEY m;
  int i, len;
  for (i=0; f->len && i < n; ++i,++f) {
    if (s) {
      switch (f->type) {
	case BT_CHAR:	ftype="char";	break;
	case BT_DATE:	ftype=(f->len < 4) ? "date" : "jul"; break;
	case BT_TIME:	ftype="time";	break;
	case BT_BIN: 	ftype="hex";	break;

	case BT_NUM:
	default:	ftype="num";	break;
      }
      sprintf(prompt,"%.10s field %d (%.10s): ",s,i,ftype);
      mesg = prompt;
    }
    p = readtext(mesg);
    if (!p || *p == '.') {	/* a period ends input & causes an early exit */
      break;
    }
    if (*p == ';') {		/* a semi-colon skips the field */
      if (f->type == BT_CHAR) {
        int l = strlen(buf) + 1;
        buf += (l > f->len) ? f->len : l;
      }
      else buf += f->len;
      continue;
    }
    /* periods and semi-colons can be escaped with a backslash */
    else if (*p == '\\' && p[1] && (p[1] == '.' || p[1] == ';')) p++;
    switch (f->type) {
    case BT_CHAR:
      len = packstr(p,p,f->len);
      (void)strncpy(buf,p,len);
      buf += len;
      break;
    case BT_DATE:
      if (f->len < 4)
        stD(stoD(p),buf);
      else 
	stJ(stoJ(p),buf);
      buf += f->len;
      break;
    case BT_TIME:
      stlong(stoT(p),buf);
      buf += f->len;
      break;
    case BT_BIN:
      hexs2bin(p,buf,f->len);
      buf += f->len;
      break;
    case BT_NUM:
    default:
      (void)strtomny(",.-",p,&m,f->type - BT_NUM);
      stnum(m,buf,f->len);
      buf += f->len;
      break;
    }
  }
  return buf - obuf;
}

int patch() {
  char *s = readtext("Filename to patch: ");
  BTCB *dtf;
  struct btflds fcb;
  int rc,klen,rlen,patchit,lastklen = 0;
  char lbuf[MAXREC];

  if (!s) return 1;
  dtf = btopen(s,BTRDWR,MAXREC);
  if (!dtf) {
    (void)printf("%s: not found\n",s);
    return 0;
  }
  (void)ldflds(&fcb,dtf->lbuf,dtf->rlen);
  klen = bt_klen(dtf->lbuf);		/* calculate key length */
  rlen = bt_rlen(dtf->lbuf,dtf->rlen);	/* calculate total length */

  for (;;) {
    dtf->klen = getdata("Key",dtf->lbuf,fcb.f,fcb.klen);
    dtf->rlen = MAXREC;			/* get all of record */
    if (dtf->klen == 0) {
      int mode = BTREADGT;
      dtf->klen = lastklen;
      if (dtf->klen == 0) mode = BTREADF;
      if (rc = btas(dtf,mode+NOKEY)) {
	puts("(EOF)");
	break;
      }
    }
    else {
      if (rc = btas(dtf,BTREADEQ+NOKEY)) {
	if (patchit = question("Record not found, add it? ")) {
          b2urec(fcb.f,lbuf,rlen,dtf->lbuf,dtf->rlen);	/* load it */
	  memset(lbuf+klen,0,rlen-klen);	/* clear data */
	}
      }
    }
    lastklen = dtf->klen;
    if (lastklen == 0) lastklen = klen;
    if (rc == 0) {
      dumprec(dtf->lbuf,dtf->rlen,fcb.f);
      if ((patchit = question("Patch record? "))) {
        b2urec(fcb.f,lbuf,rlen,dtf->lbuf,dtf->rlen);	/* load data */
      }
      else if (question("Delete record? ")) (void)btas(dtf,BTDELETE);
    }
    if (patchit) {
      getdata("Data",lbuf+klen,fcb.f+fcb.klen,fcb.rlen-fcb.klen);
      u2brec(fcb.f,lbuf,rlen,dtf,klen);
      dumprec(dtf->lbuf,dtf->rlen,fcb.f);
      if (question("Ok to patch? "))
	if (btas(dtf,BTWRITE+DUPKEY)) (void)btas(dtf,BTREPLACE);
    }
    if (question("Another patch? ") == 0) break;
  }

  (void)btclose(dtf);
  return 0;
}
