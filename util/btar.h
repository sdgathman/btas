#if 0
class btar {
  long lastpos;
  FILE *f;		/* the current archive file */
  short pathlen;
  bool seekable;
  char pathname[MAXKEY];
  struct ar_hdr;		/* archive header */
  void getbuf(void *,int);
  int gethdr(ar_hdr *,void *,int);
  int getdata(char *,int);
  void putbuf(const void *,int);
  void puthdr(ar_hdr *,const void *,int);
  void putdata(const char *,int,int);
  void puteof();
  static int arcone(const char *,const struct btstat * = 0);
public:
  btar(const char *osfile,char seekable = 0);
  int add(const char *file,bool dironly,bool subdirs);
  ~btar();
};
#endif

int btar_opennew(const char *arfile,int seekflag);
int btar_close(void);
int btar_add(const char *file,int dirflag,int subdirs);

struct btstat;
typedef int loadf(const char *dir,const char *rec,int ln,const struct btstat *);
int btar_load(const char *t,loadf *userf);
int btar_extract(const char *dir,const char *rec,int ln,const struct btstat *);
int btar_extractx(const char *dir,const char *rec,int ln,const struct btstat *,
		  int flags);
long btar_skip(long recs);
const char *permstr(short mode);

enum {
  FLAG_EXPAND = 1, FLAG_REPLACE = 2, FLAG_DIRONLY = 4, FLAG_FIELDS = 8,
  FLAG_FOUND = 16
};
