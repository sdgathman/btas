/* 1.4	REWRITE and REPLACE of directory records fixed */
/* 1.4.1 improved 209 error logging */	
/* 1.5	safe EOF detection for expandable extents */
/* 1.5.1 command line flag to disable auto-flush in unix server */
/* 1.6  safer dir unlinks, ifdef code for SAFELINK logic */ 
/* 1.7 fix partial key read bug & clean up some interfaces */
/* 1.8 independent filesystem mount, improve error logging */
/* 1.9 track btget assertion errors */
/* 1.10 BTSTAT with pathname */
/* 1.11 sequential read heuristic (BLK_LOW) */
/* 1.12 freeze command */
/* 1.13 don't include PTR in b->rlen on delrec, addrec, BTREPLACE */
/* 1.14 another partial key read bug (typo) */
/* 2.00 failsafe */
/* 2.01 201 errors calling btflush, hitting assert(lastcnt == 0) */
/* 2.02 improve auto-flush logic, package in alarm.cc */
/* 2.03 too small checkpoints cause core dump */
/* 2.04 btserve occasionally failed with SIGALRM due to timing bug */
/* 2.05 C++ encapsulation, spread writes for smoother performance
	finish updating chkpnt fs on unmount */
/* 2.06 support LOCK and BTRELEASE */
/* 2.07 safer purging of touched list, increase default MAXFLUSH */
/* 2.08 fix symlink support */
/* 2.09 don't log symlink notices */
/* 2.10 make .. cross join table */
/* 2.11+ See btas.spec */
#include "btbuf.h"
const char version[] = "\
BTAS/X v2.14, Copyright 1985-2024 Business Management Systems, Inc.\n\
";
const int version_num = 214;
