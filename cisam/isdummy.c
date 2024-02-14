#include "cisam.h"
#define ENOTDONE 123

/*
	Stubs for C-isam routines not yet implemented.  These
	will always return a fatal error code.

	The following are straightforward and should be done soon:

	isaudit
*/

int isaudit(int fd,char *name,int mode)
{
  return iserr(ENOTDONE);
}
