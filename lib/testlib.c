#include <stdlib.h>
#include <ftype.h>
#include <btflds.h>
#include <bterr.h>
#include <errenv.h>
#include <check.h>

static int verbose;
static int cnt = 1000;
static const char dir[] = "/tmp";
static const char fname[] = "/tmp/testfile";

static const char ftbl[] = {
  0,3,BT_NUM,4,BT_NUM,2, BT_BIN, 26 ,BT_BIN,200,BT_BIN,204,BT_NUM,4
};

START_TEST(test_findnone) {
  struct btff ff;
  int rc;
  btkill(fname);
  putenv("BTASDIR"); // test findfirst() with null BTASDIR
  rc = findfirst(fname,&ff);
  fail_unless(rc != 0,"testfile already present");
  finddone(&ff);
} END_TEST

START_TEST(test_findone) {
  int rc;
  struct btff ff;
  struct btflds *fld;
  putenv("BTASDIR=/"); // test findfirst() with null BTASDIR
  btkill(fname);
  fld = ldflds(0,ftbl,sizeof ftbl);
  rc = btcreate(fname,fld,0644);
  free(fld);
  fail_unless(rc == 0,"failed to create testfile");
  rc = findfirst(fname,&ff);
  fail_unless(rc == 0,"testfile not found");
  fail_unless(strcmp(ff.b->lbuf,basename(fname)) == 0,"filename mismatch");
  rc = findnext(&ff);
  fail_unless(rc != 0,"findnext fails to terminate");
  finddone(&ff);
} END_TEST

START_TEST(test_replace) {
  int rc;
  static const char ftbl2[] = { 0,1,BT_CHAR,12,BT_NUM,4 };
  struct btflds *fld;
  int i;
  BTCB *b = 0;
  char msg[80];
  putenv("BTASDIR=/"); // test findfirst() with null BTASDIR
  btkill(fname);
  fld = ldflds(0,ftbl2,sizeof ftbl2);
  rc = btcreate(fname,fld,0644);
  free(fld);
  fail_unless(rc == 0,"failed to create testfile");
  catch(rc)
    b = btopen(fname,BT_READ|BT_UPDATE,16);
    for (i = 0; i < 50; ++i) {
      b->lbuf[0] = 0;
      stlong(i,b->lbuf + 1);
      b->klen = b->rlen = 5;
      btas(b,BTWRITE);
    }
    b->lbuf[0] = 0;
    stlong(20L,b->lbuf+1);
    b->klen = 1; b->rlen = 5;
    rc = btas(b,BTREPLACE|DUPKEY);
    sprintf(msg,"Expected DUPKEY, got %d",rc);
    fail_unless(rc == BTERDUP,msg);
    // now split the root node and try again
    for (i = 50; i < 500; ++i) {
      strcpy(b->lbuf,"TESTREPLACE");
      stlong(i,b->lbuf + strlen(b->lbuf) + 1);
      b->klen = b->rlen = strlen(b->lbuf) + 5;
      btas(b,BTWRITE);
    }
    b->lbuf[0] = 0;
    stlong(20L,b->lbuf+1);
    b->klen = 1; b->rlen = 5;
    rc = btas(b,BTREPLACE|DUPKEY);
    sprintf(msg,"Expected DUPKEY, got %d",rc);
    fail_unless(rc == BTERDUP,msg);
    btclose(b);
  enverr
    sprintf(msg,"exception code %d",rc);
    btclose(b);
    fail_unless(0,msg);
  envend
} END_TEST

START_TEST(test_mkdir) {
  int rc;
  char msg[80];
  btrmdir("/testdir");
  rc = btmkdir("/testdir",0700);
  sprintf(msg,"btmkdir failed, got %d",rc);
  fail_unless(rc == 0,msg);
} END_TEST

/* Collect all the tests.  This will make more sense when tests are
 *  * in multiple source files. */
Suite *libbtas_suite (void) {
  Suite *s = suite_create ("LIBBTAS");
  TCase *tc_api = tcase_create ("API");

  suite_add_tcase (s, tc_api);
  tcase_add_test (tc_api, test_findnone);
  tcase_add_test (tc_api, test_findone);
  tcase_add_test (tc_api, test_replace);
  tcase_add_test (tc_api, test_mkdir);
  return s;
}

int main (int argc,char **argv) {
  int nf;
  Suite *s = libbtas_suite ();
  SRunner *sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);
  //suite_free (s);
  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
