#pragma interface
#include <VHMap.h>
#include <VHSet.h>

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
  class string {
    char *buf;
    int len;
    void init(const char *,int);
  public:
    string(const char *b=0,int n=0) { init(b,n); }
    string(const string &r) { init(r.buf,r.len); }
    void operator=(const string &);
    bool operator==(const string &) const;
    unsigned hash() const;
    int length() const { return len; }
    char operator[](int i) const { return buf[i]; }
    const char *c_str() const { return buf; }
    ~string() { delete[] buf; }
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

class LockTable {
  LockTable(const LockTable &);
  void operator=(const LockTable &);
  struct PidHead;	// defined in implementation
  VHMap<long,LockEntry *> pidtbl;
  VHSet<LockEntry::Ref> tbl;
public: 
  LockTable();
  bool addLock(const BTCB *);	// return true if lock succeeds
  void delLock(long pid);
  ~LockTable();
};
