#pragma interface
#include <string>
#include <map>
#include <set>
using namespace std;
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

class LockTable {
  LockTable(const LockTable &);
  void operator=(const LockTable &);
  struct PidHead;	// defined in implementation
  map<long,LockEntry *> pidtbl;
  set<LockEntry *> tbl;
public: 
  LockTable();
  bool addLock(const BTCB *);	// return true if lock succeeds
  void delLock(long pid);
  ~LockTable();
};
