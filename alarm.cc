#pragma implementation
#include "alarm.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "btbuf.h"

int Alarm::ticks = 0;
int Alarm::fatal = 0;
int Alarm::interval = 3;

Alarm::Alarm() {
  limit = 1;
  signal(SIGTERM,handler);
  signal(SIGPWR,handler);
#ifdef SIGDANGER
  signal(SIGDANGER,handler);
#endif
}

void Alarm::handler(int sig) {	/* ignore signals (but terminate system call) */
  if (sig == SIGALRM) {
    if (ticks) {
      ++ticks;
      signal(SIGALRM,handler);
      alarm(interval);
      time(&curtime);
    }
  }
  else fatal = sig;
}

void Alarm::enable(int secs) {
  limit = secs / interval;
  if (ticks == 0) {
    signal(SIGALRM,handler);
    alarm(interval);
    time(&curtime);
  }
  ticks = 1;
}

bool Alarm::check() {
  if (errno != EINTR || fatal) return true;
  if (ticks > limit) {
#if TRACE > 1
    fputs("Autoflush\n",stderr);
#endif
    if (btflush() == 0)
      ticks = 0;		/* turn off timer if complete */
  }
  limit = 1;	// flush rapidly while no activity
  return false;
}

Alarm::~Alarm() {
  if (fatal) {
    const char *s = "SIGNAL";
    switch (fatal) {
    case SIGTERM: s = "SIGTERM"; break;
#ifdef SIGDANGER
    case SIGDANGER: s = "SIGDANGER"; break;
#endif
    }
    fprintf(stderr,"BTAS/X shutdown: %s\n",s);
  }
}

