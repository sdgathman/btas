#include <stdlib.h>
#include <ftype.h>
#include <btflds.h>
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

/* Collect all the tests.  This will make more sense when tests are
 *  * in multiple source files. */
Suite *libbtas_suite (void) {
  Suite *s = suite_create ("LIBBTAS");
  TCase *tc_api = tcase_create ("API");

  suite_add_tcase (s, tc_api);
  tcase_add_test (tc_api, test_findnone);
  tcase_add_test (tc_api, test_findone);
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
