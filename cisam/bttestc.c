/*
	exercise C-isam emulator

	Copyright 1990 Business Management Systems, Inc.
	Author: Stuart D. Gathman
*/

static char what[] = "@(#)bttestc.c	1.1";

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <isamx.h>
#include <ischkerr.h>
#include "ftype.h"

struct trec {		/* test record */
  FLONG seq;
  FSHORT rnd;
  char  txt[32];
  char  pad[64];
  FLONG ver;		/* reverse sign of sequence */
};

struct keydesc Ktest = {
  0, 2, { { 0, 4, CHARTYPE }, { 4, 2, CHARTYPE } }
};

struct keydesc Ktxt =  {
  0, 3, { { 6,32, CHARTYPE }, { 0, 4, CHARTYPE }, { 4, 2, CHARTYPE } }
};

struct keydesc Krec;

long time_value;
void start_timer(/**/ const char * /**/), stop_timer(/**/ long /**/);
void verrec(/**/ const struct trec * /**/);

void start_timer(desc)
  const char *desc;
{
  time(&time_value);
  printf("%s started.\n",desc);
}

void stop_timer(cnt)
  long cnt;
{
  long elapsed;
  time(&elapsed);
  elapsed -= time_value;
  printf("\t%ld elapsed seconds.\n",elapsed);
  if (cnt > 1) {
    elapsed = elapsed * 10000 / cnt;
    printf("\t%ld.%01ld ms/rec\n",elapsed/10,elapsed%10);
  }
}

void verrec(rec)
  const struct trec  *rec;
{
    if (ldlong(rec->seq) != -ldlong(rec->ver))
      printf("Invalid record read, %ld.\n",ldlong(rec->seq));
}

void usage() {
  puts("C-isam emulator exercises version 1.1");
  puts("Usage:	btteste [-cef] cnt");
  puts("	-c	test character field compression");
  puts("	-f	test fast emulation (no record numbers, etc.)");
  puts("	-e	test exact emulation (default)");
  exit(1);
}

int exact = 1;
int compress = 0;

int main(argc, argv)
  char **argv;
{
  int fd, rc;
  struct trec t;
  long idx;
  long cnt = 0L;
  int errs, i;

  for (i = 1; i < argc && *argv[i] == '-'; ++i) {
    switch (*argv[i][1]) {
    case 'c':
      compress = 1;
      break;
    case 'e':
      exact = 1;
      break;
    case 'f':
      exact = 0;
      break;
    default:
      usage();
    }
  }
  if (i+1 != argc)
    usage();
  cnt = atol(argv[i]);
  if (cnt <= 0) cnt = 1000L;

  fd = isbuild("/tmp/testfile",sizeof t,&Ktest,ISINOUT + ISEXCLLOCK);
  if (fd == -1) {
    (void)printf("isbuild failed, rc = %d\n",iserrno);
    return 1;
  }

  stshort(0,t.rnd);
  memset(t.txt,' ',sizeof t.txt);
  (void)memset(t.pad,0xff,sizeof t.pad);
  srand(1);
  start_timer("Sequential write");
  for (idx = 0L; idx < cnt; ++idx) {
    stlong(idx,t.seq);
    t.txt[rand()%sizeof t.txt] = 'a' + rand() % 26;
    stlong(-idx,t.ver);	/* simple way to verify records */
    (void)chk(iswrite(fd,(char *)&t),0);
  }
  stop_timer(cnt);

  start_timer("Random write");
  for (idx = 1L; idx <= cnt; ++idx) {
    long seq;
    seq = rand() % cnt;
    stlong(seq,t.seq);
    stshort((short)idx,t.rnd);
    t.txt[rand()%sizeof t.txt] = 'a' + rand() % 26;
    stlong(-seq,t.ver);
    (void)chk(iswrite(fd,(char *)&t),0);
  }
  stop_timer(cnt);

  start_timer("Addindex");
  chk(isaddindex(fd,&Ktxt),0);
  stop_timer(cnt + cnt);

  start_timer("Sequential read with rewrec");
  idx = 0L;
  rc = chk(isread(fd,(char *)&t,ISFIRST),ISEOF);
  while (rc == 0) {
    ++idx;
    verrec(&t);
    if (exact) {
      /* isrewrec should not change key position */
      if (ldshort(t.rnd) == 0) {
	stshort(-1,t.rnd);
	chk(isrewrec(fd,isrecnum,&t),0);
      }
    }
    rc = chk(isread(fd,(char *)&t,ISNEXT),ISEOF);
  }
  stop_timer(idx);
  (void)printf("%ld records read.\n",idx);

  start_timer("Random read with isstart by primary key");
  errs = 0;
  for (idx = 0L; idx < cnt; ++idx) {
    long key = (long)rand()%cnt;
    stlong(key,t.seq);
    if (exact) {
      /* isstart sets key position */
      stshort(-1,t.rnd);
      if (!chk(isstart(fd,&Ktest,0,(PTR)&t,ISEQUAL),ISNOKEY)) {
	chk(isread(fd,(PTR)&t,ISCURR),0);
	if (ldlong(t.seq) != key || ldshort(t.rnd) != -1)
	  puts("wrong record");
	verrec(&t);
      }
      else
	++errs;
    }
    else {
      stshort(0,t.rnd);
      if (chk(isread(fd,(char *)&t,ISEQUAL),ISNOKEY))
	++errs;
      else
	verrec(&t);
    }
  }
  stop_timer(cnt);

  (void)printf("%d nokeys.\n",errs);

  if (exact) {
    /* selecting "physical" order */
    isstart(fd,&Krec,0,(PTR)&t,ISFIRST);
    start_timer("Sequential read by record number");
    idx = 0;
    while (chk(isread(fd,(PTR)&t,ISNEXT),ISEOF) == 0) {
      ++idx;
      verrec(&t);
    }
    stop_timer(idx);
    (void)printf("%ld records read.\n",idx);

    start_timer("Random read by record number");
    errs = 0;
    for (idx = 0L; idx < cnt; ++idx) {
      isrecnum = (long)rand()%(2*cnt) + 1;
      if (chk(isread(fd,(char *)&t,ISCURR),ISNOREC))
	++errs;
      else
	verrec(&t);
    }
    stop_timer(cnt);
    (void)printf("%d nokeys.\n",errs);
  }

  isstart(fd,&Ktxt,0,(PTR)&t,ISFIRST);
  start_timer("Sequential read with rewrite by secondary key");
  idx = 0;
  while (chk(isread(fd,(PTR)&t,ISNEXT),ISEOF) == 0) {
    int i;
    ++idx;
    verrec(&t);
    if (exact) {
      for (i = 0; i < sizeof t.txt; ++i)
	t.txt[i] = toupper(t.txt[i]);
      isrewrite(fd,(PTR)&t);
    }
  }
  stop_timer(idx);
  (void)printf("%ld records read.\n",idx);

  errs = 0;
  srand(1);
  memset(t.txt,' ',sizeof t.txt);
  stshort(-1,t.rnd);
  envelope
  start_timer("Random read (ISLTEQ) by secondary key");
  for (idx = 0L; idx < cnt; ++idx) {
    char txt[sizeof t.txt];
    stlong(idx,t.seq);
    t.txt[rand()%sizeof t.txt] = 'A' + rand() % 26;
    memcpy(txt,t.txt,sizeof txt);
    if (chk(isread(fd,(char *)&t,ISLTEQ),ISNOKEY))
      ++errs;
    else {
      if (memcmp(txt,t.txt,sizeof txt)) {
	printf("incorrect record found\n");
	printf("Key: %32.32s\nRec: %32.32s\n",txt,t.txt);
	exit(1);
      }
      verrec(&t);
    }
  }
  stop_timer(cnt);
  (void)printf("%d nokeys.\n",errs);
  enverr
    if (errno != 102) errpost(errno);
    printf("ISLTEQ not supported by direct C-isam\n");
  envend

  start_timer("Delete index");
  isdelindex(fd,&Ktxt);
  stop_timer(1L);

  (void)isclose(fd);

  /* see if isdelindex() worked */
  system("btutil 'di /tmp/*' 'du /tmp/testfile.idx'");
  system("ls -l /tmp");

  iserase("/tmp/testfile");
  return 0;
}
