/* This procedure allows the user to directly move a MONEY variable from an
input record buffer to the output data buffer.  A pointer to a static MONEY
variable is returned to allow the user to use addM,subM, etc.
*/
#include <money.h>

MONEY *FpicM(p,m,b)
  const FMONEY p;	/* MONEY variable in FILE format */
  const char *m;	/* mask */
  char *b;	/* buffer postion */
{ static MONEY jlsM;
  jlsM = ldM(p);
  (void)pic(&jlsM,m,b);
  return &jlsM;
}

