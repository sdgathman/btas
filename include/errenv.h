#ifndef envelope
#ifdef __cplusplus
extern "C" {
#endif
#include	<setjmp.h>
#include	<errno.h>
#ifndef MORE
#include	"port.h"
#endif
extern struct JMP_BUF { jmp_buf buf; } *errenv;
extern const char *bmsprog;
extern GOTO errpost(int) NORETURN;
extern GOTO errdesc(const char *, int) NORETURN;
extern GOTO errvdesc(int,const char *,MORE) NORETURN;
extern void syserr(int,char *,MORE);

#define errwhat(e,s) errvdesc(e,"%s %s line %d",s,what,__LINE__)

/* weird things happen if you combine the setjmp with the 'if' in 'envcatch' */
#define envcatch(rc) {				\
  struct JMP_BUF env, *envsav = errenv;		\
  rc = setjmp(env.buf);				\
  if (!rc && (errenv = &env)) {

/* normal processing goes here */

#define enverr	\
  }		\
  else {	\
    errenv = envsav;

/* Optional error cleanup processing goes here. If there is an error during
   cleanup, another error is signalled triggering the next level */

#define envend		\
  }			\
  errenv = envsav; 	\
}

#define envelope envcatch(errno)
#ifdef __cplusplus
}
#else
#define catch envcatch
#endif
#endif
