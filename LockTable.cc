// $Log$
// Revision 1.5  2005/02/08 16:16:17  stuart
// Port to ISO C++.
//
// Revision 1.4  2003/03/05 04:34:08  stuart
// Simplify STL LockTable
//
// Revision 1.3  2003/03/05 02:50:30  stuart
// Conversion to STL
//
// Revision 1.2  2001/02/28 21:48:19  stuart
// record lock table
//
#pragma implementation "LockTable.h"
#include "LockTable.h"
#include <stdio.h>
#include <errno.h>
#include <alloca.h>
#include <btas.h>
#include <signal.h>

unsigned LockEntry::hash() const {
  int len = key.length();
  const char *p = key.data();
  unsigned long __h = 0; 
  while (len-- > 0)
    __h = 5*__h + *p++;
  return root ^ mid ^ __h;
}

LockEntry::LockEntry() {
  pid = 0;
  root = 0;
  mid = 0;
  next = 0;
}

LockEntry::LockEntry(const BTCB *b):
  key(b->lbuf,b->klen)
  //,ident(b->lbuf + b->klen,b->rlen - b->klen)
{
  pid = b->msgident;
  root = b->root;
  mid = b->mid;
  next = 0;
}

LockEntry::LockEntry(const LockEntry &r): key(r.key) /*,ident(r.ident) */ {
  pid = r.pid;
  root = r.root;
  mid = r.mid;
  next = 0;
}

string LockEntry::toString() const {
  char *buf = (char *)alloca(80 + key.length() * 3);
  sprintf(buf,"pid=%d root=%X mid=%d key=%d,",
    pid,root,mid,key.length());
  int len = strlen(buf);
  for (int i = 0; i < key.length(); ++i) {
    sprintf(buf+len," %02X",key[i]);
    len += strlen(buf + len);
  }
  return string(buf,len);
}

void LockEntry::operator=(const LockEntry &r) {
  pid = r.pid;
  root = r.root;
  mid = r.mid;
  key = r.key;
  //ident = r.ident;
  // don't disturb next chain
}

LockEntry::~LockEntry() { }

bool LockEntry::operator==(const LockEntry &r) const {
  return root == r.root && mid == r.mid && key == r.key;
}

bool LockEntry::isValid() {
#ifdef errno
  return ::kill((int)pid,0) == 0 || errno != ESRCH;
#else
  return ::kill((int)pid,0) == 0 || ::errno != ESRCH;
#endif
}

typedef set<LockEntry *>::iterator lock_iter;
typedef map<long,LockEntry *>::iterator pid_iter;

LockTable::LockTable() { }
LockTable::~LockTable() { }

bool LockTable::addLock(const BTCB *b) {
  LockEntry e(b);
  lock_iter l = tbl.find(&e);
  if (l != tbl.end()) {
    LockEntry &cur = **l;
    if (cur.pid == e.pid)
      return true;		// already locked by us
    if (cur.isValid())
      return false;		// already locked by someone else
    delLock(cur.pid);		// already locked by dead process
  }
  // add new lock entry
  LockEntry *newentry = new LockEntry(e);
  tbl.insert(newentry);
  LockEntry *&first = pidtbl[newentry->pid];
  newentry->next = first;
  first = newentry;
  //fprintf(stderr,"Locked %s\n",newentry->toString().c_str());
  return true;
}

void LockTable::delLock(long pid) {
  pid_iter i = pidtbl.find(pid);
  if (i == pidtbl.end()) return;
  LockEntry *p = i->second;
  pidtbl.erase(i);
  while (p) {
    LockEntry *n = p->next;
    tbl.erase(p);	// remove reference from lock set
    //fprintf(stderr,"UnLock %s\n",p->toString().c_str());
    delete p;		// remove lock entry
    p = n;
  }
}
