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
  0, 2, { { 0, 4, CHARTYPE }, { 4, 2, CHARTYPE }, { 6, 26 , CHARTYPE } }
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
  (fail_unless(rc == 0 || iserrno == err,"unexpected cisam error"),rc)

START_TEST(bttesty) {
  long *ar = (long *)malloc(cnt * 2 * sizeof *ar);
  int i;
  int fd;
  int errs;
  int idx;
  int rc;
  struct trec t;

  for (i = 0; i < cnt; ++i)
    ar[i+cnt] = ar[i] = i + 1; /* initialize array */
  shuffle(ar,cnt * 2);

  fd = isbuild("testfile",sizeof t,&Ktest,ISINOUT + ISMANULOCK);
  fail_unless(fd >= 0,"isbuild failed");

  stshort(0,t.rnd);
  stlong(0L,t.ver);

  errs = 0;
  start_timer("Random deletes/writes",cnt);
  for (idx = 0L; idx < cnt; ++idx) {
    stlong(ar[idx],t.seq);
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
  errs = 0;
  for (i = 0L; i < idx; ++i) {
    stlong(ar[i],t.seq);
    stshort(0,t.rnd);
    fail_unless(isread(fd,(char *)&t,ISEQUAL) == 0,"random read failed");
    verrec(&t);
  }
  stop_timer(idx);

  errs = 0;
  start_timer("Random deletes/writes",cnt);
  for (idx = 0L; idx < cnt; ++idx) {
    stlong(ar[cnt + idx],t.seq);
    if (chk(isdelete(fd,(char *)&t),ENOREC)) {
      int plen = randrange(200,404,1);
      memset(t.pad,~0,plen);
      memset(t.pad+plen,0,sizeof t.pad - plen);
      stlong(-ar[cnt + idx],t.ver);
      fail_unless(iswrite(fd,(char *)&t) == 0,"write failed");
    }
  }
  stop_timer(cnt);

  isclose(fd);
} END_TEST

/* Collect all the tests.  This will make more sense when tests are
 *  * in multiple source files. */
Suite *cisam_suite (void) {
  Suite *s = suite_create ("CISAM");
  TCase *tc_api = tcase_create ("API");

  suite_add_tcase (s, tc_api);
  tcase_add_test (tc_api, bttesty);
  return s;
}

int main (void) {
  int nf;
  Suite *s = cisam_suite ();
  SRunner *sr = srunner_create (s);
  srandom(5910911L);
  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);
  suite_free (s);
  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
