typedef struct fld {
  char *name, *data, *format, *desc;
  int type,put;		/* actual type, get/put type */
  struct fld *next;
  short len,dlen;	/* actual length, display length */
} FLD;

struct file {
  char *name;	/* basename of file */
  char *buf;	/* data buffer */
  int rlen;	/* record length */
  int  fd;	/* cisam file descriptor */
  struct keydesc *klist;
  FLD  *flist;	/* list of field descriptors */
};

extern FLD *fcbread(/**/ const char *, char *, int /**/);
extern void fcbfree(/**/ FLD * /**/);

extern FLD *btfread(/**/ char *, char *, int /**/);

extern FLD *ddread(/**/ char *, char *, int /**/);
