#include <string.h>
#include <btas.h>
#include "btutil.h"

const char *permstr(short mode) {
  static char pstr[11];
  static struct {
    short mask;
    char  disp[2];
  } perm[10] = {
    { BT_DIR, "-d" },
    { 0400, "-r" }, { 0200, "-w" }, { 0100, "-x" },
    { 0040, "-r" }, { 0020, "-w" }, { 0010, "-x" },
    { 0004, "-r" }, { 0002, "-w" }, { 0001, "-x" }
  };
  int i;

  (void)memset(pstr,'-',10);
  for (i = 0; i < 10; ++i)
    pstr[i] = perm[i].disp[(mode&perm[i].mask)!=0];
  return pstr;
}
