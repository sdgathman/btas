#include <stdlib.h>
#include <isamx.h>
#include <ftype.h>
#include <btflds.h>

#define iserr(code)	((iserrno = code) ? -1 : 0)
enum { MAXFILES = 63, MAXCISAMREC = 4096, MAXKEYNAME = 32 };
#define MAXFILES	63
#define MAXCISAMREC	4096
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

struct cisam_key {
  BTCB *btcb;
  struct btflds *f;
  struct cisam_key *next;	/* next key */
  short klen;			/* cisam key length */
  struct keydesc k;		/* cisam keydesc (not normalized) */
  char name[MAXKEYNAME + 1];
/* save cmprec results in k_rootnode since we don't use it elsewhere */
#define cmpresult k.k_rootnode
};

struct cisam {
  struct cisam_key *curidx;	/* current index selected */
  struct cisam_key *recidx;	/* recnum index, 0 if disabled */
  BTCB *idx;			/* control file */
  struct btflds *f;		/* idx field table if EXCL */
  BTCB *dir;			/* idx directory if EXCL */
  short rlen;			/* users record size */
  short klen;			/* user partial key length */
  short start;			/* isstart() mode (yechh!) */
  struct cisam_key key;		/* primary key, chained to secondaries */
};
/* use unused part of recidx keydesc for record numbers */
#define c_recno(r) (*(long *)r->recidx->k.k_part)

extern struct cisam *isamfd[MAXFILES];		/* max cisam files open */
struct btflds *isconvkey(const struct btflds *, struct keydesc *, int);
void iskeynorm(struct keydesc *);
int iskeycomp(const struct keydesc *, const struct keydesc *);
void isstkey(const char *, const struct keydesc *, struct fisam *);
void isldkey(char *, struct keydesc *, const struct fisam *);
int ismaperr(int);		/* map error codes */
int isrlen(const struct btflds *);
