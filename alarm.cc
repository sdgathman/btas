/* $Log$
    Alarm class monitors time to auto-flush buffers and signals a shutdown
    for fatal signals.
    Copyright (C) 1985-2013 Business Management Systems, Inc

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#pragma implementation
#include "alarm.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "btserve.h"

int Alarm::ticks = 0;
int Alarm::fatal = 0;
int Alarm::interval = 3;

Alarm::Alarm() {
  limit = 1;
  signal(SIGTERM,handleFatal);
  signal(SIGPWR,handleFatal);
#ifdef SIGDANGER
  signal(SIGDANGER,handleFatal);
#endif
}

void Alarm::handleFatal(int sig) {
  fatal = sig;
}

void Alarm::handler(int sig) {	/* ignore signals (but terminate system call) */
  signal(SIGALRM,handler);
  if (ticks) {
    ++ticks;
    alarm(interval);
    time(&btserve::curtime);
  }
}

/* The main server loop calls this after every operation.  It must be 
   fast most of the time.  Secs is the maximum age of dirty buffers.  For now,
   we enforce this by forcing a flush at that interval.  Normally we
   flush only when there is no activity as detected by calls to check().
 */
void Alarm::enable(int secs) {
  limit = secs / interval;
  if (ticks == 0) {
    ticks = 1;
    handler(SIGALRM);
  }
  else
    ticks = 1;
}

/* The main server loop calls this every time a blocking system call
   fails - usually because msgrcv() is interrupted by SIGALRM.  
   Return true if the server should shutdown.
 */
bool Alarm::check(btserve *server) {
  if (errno != EINTR || fatal) return true;
  if (ticks > limit) {
    //fputs("Autoflush\n",stderr);
    if (server->flush() == 0)
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
    fprintf(stderr,"BTAS/X shutdown: %s at %s",s,ctime(&btserve::curtime));
  }
}
