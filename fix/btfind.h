// We reinvent this wheel because the findfirst() functions change
// BTASDIR, which screws up rcvr::~rcvr() (it needs to open the parent
// directory.

class btFind {
  struct BTCB *b;
  const char *dir;
  const char *pat;
  short slen;
public:
  btFind(const char *path);
  void path(char *buf) const;
  const char *name() const { return b->lbuf; }
  int first();
  int next();
  ~btFind() { btclose(b); }
};

