#pragma interface

class btserve {
  class BlockCache *bufpool;
  class btfile *engine;
  class LockTable *locktbl;
  long iocnt;
public:
  btserve(int maxblk,unsigned cachesize,char id = 0);
  int btas(struct BTCB *b,int op);
  int getmaxrec() const;
  int mount(const char *s);
  void sync();
  int flush();
  void setSafeEof(bool);
  void incTrans();	// increment transaction statistics
  static long curtime;
  //virtual void message(const char *msg); // display a diagnostic message
  ~btserve();
};
