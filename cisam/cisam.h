#include <stdlib.h>
#include <isamx.h>
#include <ftype.h>
#include <btflds.h>

#define iserr(code)	((iserrno = code) ? -1 : 0)
enum { MAXFILES = 255, MAXCISAMREC = 4096, MAXKEYNAME = 32 };
#define ischkfd(fd)	((fd < 0 || fd >= MAXFILES)?0:isamfd[fd])
#define CTL_EXT	".idx"	/* control file extension */

struct fisam {			/* records in emulator control file */
  char	name[MAXKEYNAME];	/* btas file name of key/primary file */
  char flag;			/* 001 for ISDUPS */
  char nparts;			/* number of key parts */
  struct {			/* repeat for each key part */
    FSHORT pos, len;
    FSHORT type;		/* invert sign if type != 0 */
  } kpart[NPARTS];
};

typedef struct cisam_key {
  BTCB *btcb;
  struct btflds *f;
  struct cisam_key *next;	/* next key */
  short klen;			// full unique key length for this key
  /* if ISDUPS used, then klen != k.k_len */
  struct keydesc k;		// cisam keydesc (not normalized)
  char name[MAXKEYNAME + 1];
/* save cmprec results in k_rootnode since we don't use it elsewhere */
#define cmpresult k.k_rootnode
} CisamKey;

typedef struct cisam {
  struct cisam_key *curidx;	/* current index selected */
  struct cisam_key *recidx;	/* recnum index, 0 if disabled */
  BTCB *idx;			/* control file */
  struct btflds *f;		/* idx field table if EXCL */
  BTCB *dir;			/* idx directory if EXCL */
  short rlen;			/* users record size */
  short klen;			/* user partial key length */
  short start;			/* isstart() mode (yechh!) */
  struct cisam_key key;		/* primary key, chained to secondaries */
  char *min, *max;		/* range */
} Cisam;
/* use unused part of recidx keydesc for record number cache. */
#define c_recno(r) ((long *)r->recidx->k.k_part)[0]
#define c_recnum(r) ((long *)r->recidx->k.k_part)[1]

extern struct cisam *isamfd[MAXFILES];		/* max cisam files open */
struct btflds *isconvkey(const struct btflds *, struct keydesc *, int);
int iskeynorm(struct keydesc *);
int iskeycomp(const struct keydesc *, const struct keydesc *);
int isCheckRange(const struct cisam *,char *buf,int op);
void isstkey(const char *, const struct keydesc *, struct fisam *);
void isldkey(char *, struct keydesc *, const struct fisam *);
int ismaperr(int);		/* map error codes */
int isrlen(const struct btflds *);
