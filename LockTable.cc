#pragma implementation "Set.h"
#include <Set.h>
#include <Set.cc>
#pragma implementation "VHSet.h"
#include <VHSet.h>
#include <VHSet.cc>
#pragma implementation "Map.h"
#include <Map.h>
#include <Map.cc>
#pragma implementation "VHMap.h"
#include <VHMap.h>
#include <VHMap.cc>
#pragma implementation "LockTable.h"
#include "LockTable.h"
#include <btas.h>

static inline unsigned int hash(const long &x) { return x; }
static inline unsigned int hash(const LockEntry::Ref &r) { return r.hash(); }

unsigned LockEntry::hash() const {
  return root ^ mid ^ key.hash();
}

unsigned LockEntry::string::hash() const {
  unsigned tot = 0;
  for (int i = 0; i < len; ++i)
    tot += buf[i];
  return tot;
}

LockEntry::LockEntry() {
  pid = 0;
  root = 0;
  mid = 0;
  next = 0;
}

void LockEntry::string::operator=(const string &r) {
  if (r.len > len) {
    delete[] buf;
    buf = new char[r.len + 1];
  }
  len = r.len;
  memcpy(buf,r.buf,len);
  buf[len] = 0;
}

void LockEntry::string::init(const char *b,int n) {
  if (b && n) {
    buf = new char[n + 1];
    len = n;
    memcpy(buf,b,n);
    buf[n] = 0;
  }
  else {
    buf = 0;
    len = 0;
  }
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

LockEntry::string LockEntry::toString() const {
  char *buf = (char *)alloca(80 + key.length() * 3);
  sprintf(buf,"pid=%d root=%X mid=%d key=%d,",
    pid,root,mid,key.length());
  int len = strlen(buf);
  for (int i = 0; i < key.length(); ++i) {
    sprintf(buf+len," %02X",key[i]);
    len += strlen(buf + len);
  }
  buf[len++] = 0;
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

bool LockEntry::string::operator==(const string &r) const {
  return len == r.len && memcmp(buf,r.buf,len) == 0;
}

bool LockEntry::operator==(const LockEntry &r) const {
  return root == r.root && mid == r.mid && key == r.key;
}

bool LockEntry::isValid() {
  return ::kill((int)pid,0) == 0 || ::errno != ESRCH;
}

template class Set<LockEntry::Ref>;
template class VHSet<LockEntry::Ref>;
template class Map<long,LockEntry *>;
template class VHMap<long,LockEntry *>;

LockTable::LockTable(): pidtbl((LockEntry *)0,100) { }
LockTable::~LockTable() { }

bool LockTable::addLock(const BTCB *b) {
  LockEntry e(b);
  Pix l = tbl.seek(&e);
  if (l != 0) {
    LockEntry &cur = *tbl(l);
    if (cur.pid == e.pid)
      return true;		// already locked by us
    if (cur.isValid())
      return false;		// already locked by someone else
    delLock(cur.pid);		// already locked by dead process
  }
  // add new lock entry
  LockEntry *newentry = new LockEntry(e);
  tbl.add(newentry);
  LockEntry *&first = pidtbl[newentry->pid];
  newentry->next = first;
  first = newentry;
  fprintf(stderr,"Locked %s\n",newentry->toString().c_str());
  return true;
}

void LockTable::delLock(long pid) {
  LockEntry *p = pidtbl[pid];
  if (!p) return;
  pidtbl.del(pid);
  while (p) {
    LockEntry *n = p->next;
    tbl.del(p);		// remove reference from lock set
    fprintf(stderr,"UnLock %s\n",p->toString().c_str());
    delete p;		// remove lock entry
    p = n;
  }
}
