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

  class Ref {
    LockEntry *lock;
  public:
    Ref(LockEntry *p = 0) { lock = p; }
    bool operator==(const Ref &r) const { return *lock == *r.lock; }
    unsigned hash() const { return lock->hash(); }
    LockEntry &operator*() const { return *lock; }
  };

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

struct hash<LockEntry::Ref> { 
    size_t operator()(const LockEntry::Ref &__s) const { return __s.hash(); }
};


class LockTable {
  LockTable(const LockTable &);
  void operator=(const LockTable &);
  struct PidHead;	// defined in implementation
  hash_map<long,LockEntry *> pidtbl;
  hash_set<LockEntry::Ref> tbl;
public: 
  LockTable();
  bool addLock(const BTCB *);	// return true if lock succeeds
  void delLock(long pid);
  ~LockTable();
};
