#include <stdlib.h>
#include <isamx.h>
#include <ftype.h>
#include <btflds.h>
#include <check.h>

static int verbose;
static int cnt = 1000;
static const char dir[] = "/tmp";
static const char fname[] = "/tmp/testfile";

struct trec {		/* test record */
  FLONG seq;
  FSHORT rnd;
  char  txt[26];
  char  pad[404];
  FLONG ver;		/* reverse sign of sequence */
};

static struct keydesc Ktest = {
  0, 3, { { 0, 4, CHARTYPE }, { 4, 2, CHARTYPE }, { 6, 26 , CHARTYPE } }
};

static const char ftbl[] = {
  0,3,BT_NUM,4,BT_NUM,2, BT_BIN, 26 ,BT_BIN,200,BT_BIN,204,BT_NUM,4
};

static long time_value;

static void start_timer(const char *desc, int n) {
  time(&time_value);
  printf("%s started - %d records.\n",desc,n);
}

static void stop_timer(long cnt) {
  long elapsed;
  time(&elapsed);
  elapsed -= time_value;
  printf("\t%ld elapsed seconds.\n",elapsed);
  if (cnt > 1) {
    elapsed = elapsed * 10000 / cnt;
    printf("\t%ld.%01ld ms/rec\n",elapsed/10,elapsed%10);
  }
}

double drand() { return random() / (RAND_MAX + 1.0); }

int randrange(int start,int stop,int step) {
  int n;
  if (step > 0)
    n = (stop - start + step - 1) / step;
  else if (step < 0)
    n = (stop - start + step + 1) / step;
  else
    fail("zero step for randrange");
  return start + step * (int)(drand() * n);
}

static void shuffle(long *ar,int size) {
  int i;
  for (i = size-1; i >= 0; --i) {
    int j = (int)(drand() * (i+1));
    int t = ar[i];
    ar[i] = ar[j];
    ar[j] = t;
  }
}

static void verrec(const struct trec *rec) {
  fail_unless(ldlong(rec->seq) == -ldlong(rec->ver),"Invalid record read.");
}

#define chk(rc,err) \
  ((rc == 0) ? 0 : (fail_unless(iserrno == err,"unexpected cisam error"),-1))

START_TEST(bttesty) {
  long *ar = (long *)malloc(cnt * 2 * sizeof *ar);
  int i;
  int fd;
  int idx;
  int rc;
  struct trec t;

  for (i = 0; i < cnt; ++i)
    ar[i+cnt] = ar[i] = i + 1; /* initialize array */
  shuffle(ar,cnt * 2);

  iserase(fname);
  fd = isbuild(fname,sizeof t,&Ktest,ISINOUT + ISMANULOCK);
  fail_unless(fd >= 0,"isbuild failed");

  start_timer("\nRandom deletes/writes",cnt);
  for (idx = 0L; idx < cnt; ++idx) {
    stlong(ar[idx],t.seq);
    stshort(0,t.rnd);
    stchar("bttesty",t.txt,sizeof t.txt);
    if (chk(isdelete(fd,(char *)&t),ENOREC)) {
      int plen = randrange(200,404,1);
      memset(t.pad,~0,plen);
      memset(t.pad+plen,0,sizeof t.pad - plen);
      stlong(-ar[idx],t.ver);
      fail_unless(iswrite(fd,(char *)&t) == 0,"dupkey on write");
    }
  }
  stop_timer(cnt);

  start_timer("Sequential read",cnt);
  idx = 0L;
  rc = chk(isread(fd,(char *)&t,ISFIRST),EENDFILE);
  while (rc == 0) {
    if (idx < cnt) ar[idx++] = ldlong(t.seq);
    verrec(&t);
    rc = chk(isread(fd,(char *)&t,ISNEXT),EENDFILE);
  }
  stop_timer(idx);
  printf("\t%ld records read.\n",idx);
  fail_unless(idx <= cnt,"too many records read");

  shuffle(ar,idx);
  start_timer("Random read",idx);
  for (i = 0L; i < idx; ++i) {
    stlong(ar[i],t.seq);
    stshort(0,t.rnd);
    fail_unless(isread(fd,(char *)&t,ISEQUAL) == 0,"random read failed");
    verrec(&t);
  }
  stop_timer(idx);

  start_timer("Random deletes/writes",cnt);
  for (idx = 0L; idx < cnt; ++idx) {
    stlong(ar[cnt + idx],t.seq);
    stshort(0,t.rnd);
    stchar("bttesty",t.txt,sizeof t.txt);
    if (chk(isdelete(fd,(char *)&t),ENOREC)) {
      int plen = randrange(200,404,1);
      memset(t.pad,~0,plen);
      memset(t.pad+plen,0,sizeof t.pad - plen);
      stlong(-ar[cnt + idx],t.ver);
      fail_unless(iswrite(fd,(char *)&t) == 0,"write failed");
    }
  }
  stop_timer(cnt);
  free(ar);

  { struct dictinfo di;
    fail_unless(isindexinfo(fd,(struct keydesc *)&di,0) == 0,
	"indexinfo failed");
    if (di.di_nrecords != 0)
      printf("%d records remaining\n",di.di_nrecords);
    fail_unless(di.di_nrecords == 0,"file not empty");
  }

  fail_unless(isclose(fd) == 0,"close failed");
  fail_unless(iserase(fname) == 0,"erase failed");

} END_TEST

START_TEST(bttestx) {
  int i;
  long *ar = (long *)malloc(cnt * sizeof *ar);
  int fd;
  int rc;
  int idx;
  struct trec t;

  for (i = 0;i < cnt;i++) ar[i] = i + 1;	/* initialize array */
  shuffle(ar,cnt);

  fd = isbuild(fname,sizeof t,&Ktest,ISINOUT + ISMANULOCK);
  fail_unless(fd >= 0,"isbuild failed");

  stshort(0,t.rnd);
  stchar("bttestx",t.txt,sizeof t.txt);
  memset(t.pad,0xff,sizeof t.pad);
  start_timer("\nSequential write",cnt);
  for (idx = 1L; idx <= cnt; ++idx) {
    stlong(idx,t.seq);
    stlong(-idx,t.ver);	/* simple way to verify records */
    fail_unless(iswrite(fd,(char *)&t) == 0,"write failed");
  }
  stop_timer(cnt);

  start_timer("Random delete",cnt);
  for (idx = 0L; idx < cnt; ++idx) {
    stlong(ar[idx],t.seq);
    fail_unless(isdelete(fd,(char *)&t) == 0,"delete failed");
  }
  stop_timer(cnt);

  { struct dictinfo di;
    fail_unless(isindexinfo(fd,(struct keydesc *)&di,0) == 0,
	"indexinfo failed");
    if (di.di_nrecords != 0)
      printf("%d records remaining\n",di.di_nrecords);
    fail_unless(di.di_nrecords == 0,"file not empty");
  }

  shuffle(ar,cnt);
  start_timer("Random write",cnt);
  for (idx = 0L; idx < cnt; ++idx) {
    long seq;
    seq = ar[idx];
    stlong(seq,t.seq);
    stshort((short)idx,t.rnd);
    stlong(-seq,t.ver);
    fail_unless(iswrite(fd,(char *)&t) == 0,"write failed");
  }
  stop_timer(cnt);

  start_timer("Sequential read",cnt);
  idx = 0L;
  rc = chk(isread(fd,(char *)&t,ISFIRST),EENDFILE);
  while (rc == 0) {
    ++idx;
    verrec(&t);
    rc = chk(isread(fd,(char *)&t,ISNEXT),EENDFILE);
  }
  stop_timer(cnt);
  printf("\t%ld records read.\n",idx);

  shuffle(ar,cnt);
  start_timer("Random read",cnt);
  for (idx = 0L; idx < cnt; ++idx) {
    stlong(ar[idx],t.seq);
    stshort(0,t.rnd);
    fail_unless(isstart(fd,&Ktest,4,(char *)&t,ISEQUAL) == 0,"isstart failed");
    fail_unless(isread(fd,(char *)&t,ISNEXT) == 0,"read NEXT failed");
    verrec(&t);
  }
  stop_timer(cnt);
  free(ar);

  start_timer("Sequential read and delete",cnt);
  idx = 0L;
  rc = chk(isread(fd,(char *)&t,ISFIRST),EENDFILE);
  while (rc == 0) {
    ++idx;
    verrec(&t);
    fail_unless(isdelcurr(fd) == 0,"delcurr failed");
    rc = chk(isread(fd,(char *)&t,ISNEXT),EENDFILE);
  }
  stop_timer(cnt);
  printf("\t%ld records deleted.\n",idx);

  { struct dictinfo di;
    fail_unless(isindexinfo(fd,(struct keydesc *)&di,0) == 0,
	"indexinfo failed");
    if (di.di_nrecords != 0)
      printf("%d records remaining\n",di.di_nrecords);
    fail_unless(di.di_nrecords == 0,"file not empty");
  }

  fail_unless(isclose(fd) == 0,"close failed");
  fail_unless(iserase(fname) == 0,"erase failed");

} END_TEST

/* Test isindexinfo() with small record lengths.  */
START_TEST(test_dictinfo) {
  static struct keydesc Ktest = { 0, 1, { { 0, 4, CHARTYPE } } };
  static const char ftbl[] = { 0,1,BT_NUM,4 };
  int fd;
  struct btflds *f;
  struct dictinfo di;
  iserase(fname);
  f = ldflds(0,ftbl,sizeof ftbl);
  fd = isbuildx(fname,4,&Ktest,ISINOUT + ISMANULOCK,f);
  free(f);
  fail_unless(isindexinfo(fd,(struct keydesc *)&di,0) == 0, "indexinfo failed");
  fail_unless(di.di_nkeys == 1,"should be only 1 index");
  fail_unless(di.di_recsize == 4,"rlen not 4");
  fail_unless(di.di_nrecords == 0,"file not empty");
  /* file should be erased later */
  fail_unless(iserase(fname) == 0,"erase failed");
  fail_unless(isclose(fd) == 0,"close failed");
  fail_unless(iscleanup(0) == 0,"cleanup failed");
} END_TEST

enum { FDLIMIT = 255 };
START_TEST(test_fdlimit) {
  int oldlimit = isfdlimit(FDLIMIT);
  int fd,i;
  int v[FDLIMIT + 10];
  iserase(fname);
  fd = isbuild(fname,100,&Ktest,ISINOUT + ISMANULOCK);
  fail_unless(fd >= 0,"isbuild failed");
  fail_unless(isclose(fd) == 0,"close failed");
  for (i = 0; i < FDLIMIT + 10; ++i) {
    fd = isopen(fname,ISINOUT + ISMANULOCK);
    if (fd < 0) break;
    v[i] = fd;
  }
  fail_unless(i == FDLIMIT,"wrong fdlimit");
  fail_unless(iserrno == ETOOMANY,"did not get ETOOMANY");
  iscloseall();
  fail_unless(iserase(fname) == 0,"erase failed");
  fail_unless(isfdlimit(oldlimit) == FDLIMIT,"restore fdlimit failed");
} END_TEST

START_TEST(test_bigrec) {
  int fd;
  char buf[1024];
  char s[37];
  fd = isopenx("/edx/AIRPEX/ANSI/SVCREQ",ISINOUT + ISMANULOCK,552);
  fail_unless(fd >= 0,"SVCREQ isopen failed");
  fail_unless(isread(fd,buf,ISFIRST) == 0,"SVCREQ isread failed");
  stchar("Testing",buf + 516,36);
  fail_unless(isrewcurr(fd,buf) == 0,"SVCREQ isrewrite failed");
  stchar("",buf + 516,36);
  fail_unless(isread(fd,buf,ISFIRST) == 0,"SVCREQ isread failed");
  ldchar(buf+516,36,s);
  fail_unless(strcmp(s,"Testing") == 0,"datacmp failed");
  isclose(fd);
} END_TEST

static struct keydesc Ktest2 = { 0, 1, { { 0, 12, CHARTYPE } } };
static const char ftbl2[] = { 0,1,BT_CHAR,12,BT_NUM,4 };

START_TEST(test_replace) {
    int fd;
    char buf[16];
    struct btflds *f;
    const char *fname = "/tmp/testreplace";
    iserase(fname);
    f = ldflds(0,ftbl2,sizeof ftbl2);
    fd = isbuildx(fname,sizeof buf,&Ktest2,ISINOUT + ISMANULOCK,f);
    free(f);
    fail_unless(fd >= 0,"isbuildx failed");
    stchar("",buf,12);
    stlong(1L,buf+12);
    fail_unless(iswrite(fd,buf) == 0,"write failed");
    fail_unless(isread(fd,buf,ISFIRST) == 0,"isread failed");
    stlong(2L,buf+12);
    fail_unless(isrewcurr(fd,buf) == 0,"isrewcurr failed with single rec");
    stlong(1L,buf+12);
    fail_unless(iswrite(fd,buf) != 0,"write should get dupkey");
    stchar("a",buf,12);
    fail_unless(iswrite(fd,buf) == 0,"write failed");
    fail_unless(isread(fd,buf,ISLAST) == 0,"isread failed");
    stchar("",buf,12);
    fail_unless(isrewcurr(fd,buf) != 0,"isrewcurr should get dupkey");
    isclose(fd);
} END_TEST

START_TEST(test_addindex) {
    int fd1 = -1, fd2 = -1;
    int rc;
    char buf[32];
    struct btflds *f = 0;
    const char *fname1 = "/tmp/testaddindex1";
    const char *fname2 = "/tmp/testaddindex2";
    const char *kname = "testaddindex_num";
    const char *abskname = "/tmp/testaddindex_num";
    static struct keydesc Ktest3 = { 0, 2, {
    	{ 12, 4, CHARTYPE },
	{ 0, 12, CHARTYPE } 
    } };
    static struct keydesc Ktest4 = { 0, 1, { { 12, 4, CHARTYPE } } };
    iserase(fname1);
    iserase(fname2);
    iserase(kname);
    f = ldflds(0,ftbl2,sizeof ftbl2);
    fd1 = isbuildx(fname1,sizeof buf,&Ktest2,ISINOUT + ISEXCLLOCK,f);
    fd2 = isbuildx(fname2,sizeof buf,&Ktest2,ISINOUT + ISEXCLLOCK,f);
    fail_unless(isaddindexn(fd1,&Ktest3,kname) == 0,"isaddindexn1 failed");
    fail_unless(isaddindexn(fd2,&Ktest3,kname) != 0,
    	"isaddindexn2 should have failed");
    fail_unless(iserrno == EKEXISTS,
    	"isaddindexn2 should have iserrno == EKEXISTS");
    fail_unless(isaddindexn(fd1,&Ktest3,kname) != 0,
    	"isaddindexn1 should have failed on dup index");
    fail_unless(iserrno == EKEXISTS,
    	"isaddindexn2 should have iserrno == EKEXISTS");
    fail_unless(isindexname(fd1,buf,2) == 0,"isindexname failed");
    fail_unless(buf[0] != '/',"indexname starts with slash");
    // add some records for testing index autorepair
    stchar("foo",buf,12);
    stlong(1,buf+12);
    fail_unless(iswrite(fd1,buf) == 0,"iswrite failed");
    stchar("bar",buf,12);
    stlong(0,buf+12);
    fail_unless(iswrite(fd1,buf) == 0,"iswrite failed");
    isclose(fd1);
    isclose(fd2);
    // now, corrupt the key file
    fd1 = isopenx(abskname,ISINOUT + ISMANULOCK,16);
    fail_unless(fd1 >= 0,"isopenx failed");
    fail_unless(isread(fd1,buf,ISFIRST) == 0,"isfirst failed");
    fail_unless(ldlong(buf) == 0,"expected 0");
    stlong(2,buf);
    fail_unless(isrewcurr(fd1,buf) == 0, "isrewcurr failed");
    isclose(fd1);
    // now, reopen master and try to update "bar" record
    fd1 = isopenx(fname1,ISINOUT + ISMANULOCK,sizeof buf);
    fail_unless(fd1 >= 0,"isopenx failed");
    fail_unless(isread(fd1,buf,ISFIRST) == 0,"isfirst failed");
    fail_unless(ldlong(buf+12) == 0,"expected 0");
    stlong(2,buf+12);
    fail_unless(isrewcurr(fd1,buf) == 0, "auto-repair failed");
    isclose(fd1);

    // replace index with non-dups index
    fd1 = isopenx(fname1,ISINOUT + ISEXCLLOCK,sizeof buf);
    fail_unless(fd1 >= 0,"isopenx failed");
    fail_unless(isdelindex(fd1,&Ktest3) == 0, "isdelindex failed");
    fail_unless(isaddindexn(fd1,&Ktest4,kname) == 0,"isaddindexn1 failed");
    stchar("baz",buf,12);
    stlong(2,buf+12);
    fail_unless(iswrite(fd1,buf) != 0 && iserrno == EDUPL,
      "iswrite should have failed with dup key");
    stlong(3,buf+12);
    fail_unless(iswrite(fd1,buf) == 0,"iswrite failed");
    isclose(fd1);
} END_TEST

START_TEST(test_erasedir) {
  const char *name1 = "/tmp/testerase";
  btmkdir(name1,0700);
  fail_unless(iserase(name1) != 0,"iserase shouldn't delete directories");
  fail_unless(btrmdir(name1) == 0,"btrmdir failed");
} END_TEST

/* Collect all the tests.  This will make more sense when tests are
 *  * in multiple source files. */
Suite *cisam_suite (void) {
  Suite *s = suite_create ("CISAM");
  TCase *tc_api = tcase_create ("API");

  suite_add_tcase (s, tc_api);
  tcase_add_test (tc_api, test_fdlimit);
  tcase_add_test (tc_api, bttestx);
  tcase_add_test (tc_api, bttesty);
  tcase_add_test (tc_api, test_dictinfo);
  //tcase_add_test (tc_api, test_bigrec);
  tcase_add_test (tc_api, test_replace);
  tcase_add_test (tc_api, test_addindex);
  tcase_add_test (tc_api, test_erasedir);
  return s;
}

int main (int argc,char **argv) {
  int nf;
  Suite *s = cisam_suite ();
  SRunner *sr = srunner_create (s);
  if (argc > 1)
    cnt = atoi(argv[1]);
  srandom(5910911L);
  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);
  //suite_free (s);
  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
