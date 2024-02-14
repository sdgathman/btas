/* $Log$
 * Revision 1.4  1998/11/03  20:43:25  stuart
 * support deleting indexes by name
 *
 * Revision 1.3  1997/05/08  17:14:30  stuart
 * support named indexes for addindex
 *
 */
static const char what[] = "$Revision$";
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <isamx.h>
#include <errenv.h>
#include <ischkerr.h>
#include <btas.h>

#define count(a)  (sizeof a/sizeof a[0])

void usage() {
    fprintf(stderr,"$Id$\n\
Usage:	%s [-d] [-n filename] filename offset length [offset length ...]\n\
	-d	set ISDUPS for this index\n\
	-n name	name this index, by default a serial number is used\n\
", bmsprog);
}

int main(int argc,char **argv) {
  int err = 0, fd = -1, i;
  const char *fname;
  const char *idxname = 0;
  char dupkey = 0;
  struct keydesc key;
  bmsprog = basename(argv[0]);
  argc--; argv++;
  if (argc <= 0) {
    usage();
    return 1;
  }
  while (argc > 0 && *argv[0] == '-') {
    if (strcmp(argv[0],"-d") == 0) {
      dupkey = ISDUPS;
      argc--; argv++;
      continue;
    }
    if (strncmp(argv[0],"-n",2) == 0) {
      idxname = argv[0] + 2;
      if (!*idxname) {
	argc--; argv++;
	idxname = argv[0];
      }
      argc--; argv++;
      continue;
    }
    usage();
    return 1;
  }
  if (--argc%2) {
    fprintf(stderr,"%s: offset is missing length\n",bmsprog);
    return 1;
  }
  fname = *argv++;
catch(err)
  chkopen(fd,fname,ISINOUT+ISEXCLLOCK);
  for (i=0; i < argc/2 && i < count(key.k_part); i++) {
    key.k_part[i].kp_start = atoi(argv[0]); argv++;
    key.k_part[i].kp_leng = atoi(argv[0]);  argv++;
    key.k_part[i].kp_type = CHARTYPE;
  }
  key.k_flags = dupkey;
  key.k_nparts = i;
  if (strncmp(bmsprog,"del",3) == 0) {
    if (idxname) {
      char buf[MAXKEYSIZE];
      for (i = 1; isindexname(fd,buf,i) == 0; ++i) {
	if (strcmp(buf,idxname) == 0) {
	  chk(isindexinfo(fd,&key,i),0);
	  i = 0;
	  break;
	}
      }
      if (i) errpost(EBADKEY);
    }
    chk(isdelindex(fd,&key),0);
  }
  else {
    chk(isaddindexn(fd,&key,idxname),0);
  }
enverr
  switch (iserrno) {
  case EBADKEY:
    fprintf(stderr,"%s: '%s' key not found\n",bmsprog,fname);
    break;
  case EKEXISTS:
    fprintf(stderr,"%s: '%s' key already exists\n",bmsprog,fname);
    break;
  case ENOREC:
    fprintf(stderr,"%s: '%s' not found\n",bmsprog,fname);
    break;
  case EDUPL:
    fprintf(stderr,"%s: '%s' has a DUPKEY on the new index\n",bmsprog,fname);
    break;
  case ENOTEXCL:
    fprintf(stderr,"%s: '%s' needs exclusive access to modify indexes\n",
	bmsprog,fname);
    break;
  default:
    fprintf(stderr,"%s: Cisam failure, iserrno = %d\n",bmsprog,iserrno);
  }
envend
  iscloseall();
  return 0;
}
