#pragma interface
#include <string>
#include <hash_map>
#include <hash_set>

struct BTCB;

struct LockEntry {
  LockEntry();
  LockEntry(const LockEntry &);
  LockEntry(const BTCB *b);
  bool operator==(const LockEntry &) const;
  void operator=(const LockEntry &);
  bool isValid();
  long getPid() const { return pid; }
  unsigned hash() const;
  ~LockEntry();

  string toString() const;
private:
  long root;
  short mid;
  string key;
  //string ident;	// identifying information for debugging locks
  LockEntry *next;
  long pid;
  friend class LockTable;
};

struct hash<LockEntry *> {
  size_t operator()(LockEntry * const &__s) const { return __s->hash(); }
};

struct equal_to<LockEntry *> {
  bool operator()(LockEntry * const &a,LockEntry * const &b) const {
    return *a == *b;
  }
};

class LockTable {
  LockTable(const LockTable &);
  void operator=(const LockTable &);
  struct PidHead;	// defined in implementation
  hash_map<long,LockEntry *> pidtbl;
  hash_set<LockEntry *> tbl;
public: 
  LockTable();
  bool addLock(const BTCB *);	// return true if lock succeeds
  void delLock(long pid);
  ~LockTable();
};
