/*	Directory tables - partly finished, Author: Stuart D. Gathman
 * $Log$
 */
#include <stdio.h>
#ifndef __MSDOS__
#include <pwd.h>
#include <grp.h>
#include <sys/utsname.h>
#endif
#include <time.h>
#include <string.h>
#include <stddef.h>
#include <errenv.h>
#include <bterr.h>
#include "cursor.h"
#include "coldate.h"
#include <date.h>
#include <btflds.h>
#include <ftype.h>
#include "config.h"

enum dirField {
  dirName,
  dirRoot,
  dirMid,
  dirUser,
  dirGroup,
  dirPerm,
  dirUid,
  dirGid,
  dirMtime,
  dirAtime,
  dirCtime,
  dirLinks,
  dirLocks,
  dirRecs,
  dirBlks,
  dirMode,
  dirRlen,
  dirKlen,
#if 0
  dirFlds,
  dirKflds,
#endif
  dirNcols
};

typedef struct DirField {
  struct Column_class *_class;
/* public: */
  char *name;		/* field name */
  char *desc;		/* description */
  char *buf;		/* buffer address */
  unsigned char len;	/* field length */
  char type;		/* type code */
  short dlen;		/* display length */
  short width;		/* column width (room for dlen & strlen(desc) */
  short tabidx, colidx;	/* hack for quick testing */
/* private: */
  struct Directory *dir;
  enum dirField fld;
} DirField;

typedef struct Directory {
  struct Cursor_class *_class;
/* public: */
  short ncol;	/* number of columns */
  short klen;	/* number of key columns */
  Column **col;
  long rows;	/* estimated number of rows */
  long cnt;	/* actual returned so far */
/* private: */
  char namekey[32];	/* save name for Directory_find */
  char stat;		/* true if status read for this record */
  BTCB *dir;
  struct btstat st;	/* file status */
  struct btlevel loc;	/* file location */
/* Directory columns  */
  struct DirField dcols[dirNcols];
} Directory;

static struct {
  char *name;
  char *desc;
  int offset;
  short width;
  unsigned char len;
  char type;
} dirTable[dirNcols] = {
  { "name", "Name",	offsetof(Directory,namekey),	32, 32, BT_CHAR },
  { "root", "Root",	offsetof(Directory,loc.node),	8,  4,  BT_NUM },
  { "mid",  "Mid",  	offsetof(Directory,loc.slot),	3,  2,  BT_NUM },
  { "user", "User",  	offsetof(Directory,st.id.user),	8,  8,  BT_CHAR },
  { "group","Group",  	offsetof(Directory,st.id.group),8,  8,  BT_CHAR },
  { "perm", "Permission",offsetof(Directory,st.id.mode),10, 10,  BT_CHAR },
  { "uid",  "Uid",  	offsetof(Directory,st.id.user),	5,  2,  BT_NUM },
  { "gid",  "Gid",  	offsetof(Directory,st.id.group),5,  2,  BT_NUM },
  { "mtime","Modified", offsetof(Directory,st.mtime),	12, 4,  BT_TIME },
  { "atime","Accessed", offsetof(Directory,st.atime),	12, 4,  BT_TIME },
  { "ctime","Changed",  offsetof(Directory,st.ctime),	12, 4,  BT_TIME },
  { "links","Links",	offsetof(Directory,st.links),	5,  2,  BT_NUM },
  { "locks","Locks",	offsetof(Directory,st.opens),	5,  2,  BT_NUM },
  { "recs", "Records",  offsetof(Directory,st.rcnt),	8,  4,  BT_NUM },
  { "blks", "Blocks",  	offsetof(Directory,st.bcnt),	8,  4,  BT_NUM },
  { "mode", "Mode",  	offsetof(Directory,st.id.mode), 5,  2,  BT_NUM },
  { "rlen", "Rlen",  	0, 5,  2,  BT_NUM },
  { "klen", "Klen",  	0, 5,  2,  BT_NUM },
#if 0
  { "flds", "Flds",  	0, 5,  2,  BT_NUM },
  { "kflds", "Kflds",  	0, 5,  2,  BT_NUM }
#endif
};

static char *permstr(int mode) {
  static char pstr[11];
  static struct {
    short mask;
    char  disp[2];
  } perm[10] = {
    BT_DIR, "-d",
    0400, "-r", 0200, "-w", 0100, "-x",
    0040, "-r", 0020, "-w", 0010, "-x",
    0004, "-r", 0002, "-w", 0001, "-x",
  };
  register int i;

  memset(pstr,'-',10);
  for (i = 0; i < 10; ++i)
    pstr[i] = perm[i].disp[(mode&perm[i].mask)!=0];
  return pstr;
}

static const char *pwName(int uid) {
  static char buf[18];
#ifndef __MSDOS__
  static struct passwd *pw;
  if (!pw || pw->pw_uid != uid)
    pw = getpwuid(uid);
  if (pw)
    return pw->pw_name;
#endif
  sprintf(buf,"%d",uid);
  return buf;
}

static const char *grName(int gid) {
  static char buf[18];
#ifndef __MSDOS__
  static struct group *gr;
  if (!gr || gr->gr_gid != gid)
    gr = getgrgid(gid);
  if (gr)
    return gr->gr_name;
#endif
  sprintf(buf,"%d",gid);
  return buf;
}

static void Directory_stat(Directory *dir) {
  if (!dir->stat) {
    dir->dir->klen = strlen(dir->dir->lbuf);
    btlstat(dir->dir,&dir->st,&dir->loc);
    dir->stat = 1;
  }
}

static const char ftmask[] = "Nnn DD CCCCC";

static void DirField_print(Column *c,enum Column_type what,char *buf) {
  if (what < VALUE)
    Column_print(c,what,buf);
  else {
    DirField *this = (DirField *)c;
    Directory *dir = this->dir;
    int i;
    if (this->fld != dirName)
      Directory_stat(dir);
    switch (this->fld) {
    case dirName:
      sprintf(buf,"%-32.32s",this->buf); break;
    case dirRoot:
      sprintf(buf,"%08X",dir->loc.node); break;
    case dirMid:
      sprintf(buf,"%3d",dir->loc.slot); break;
    case dirUser:
      sprintf(buf,"%-8s",pwName(dir->st.id.user)); break;
    case dirGroup:
      sprintf(buf,"%-8s",grName(dir->st.id.group)); break;
    case dirPerm:
      sprintf(buf,"%10s",permstr(dir->st.id.mode)); break;
    case dirBlks: case dirRecs:
      sprintf(buf,"%8ld",*(long *)this->buf); break;
    case dirUid: case dirGid:
    case dirLinks: case dirLocks:
      sprintf(buf,"%5d",*(short *)this->buf); break;
    case dirMode:
      sprintf(buf,"%5o",dir->st.id.mode); break;
    case dirMtime: case dirAtime: case dirCtime:
      timemask(*(long *)this->buf,ftmask,buf);
      sprintf(buf,"%12.12s",buf);
      break;
    case dirRlen:
      if (dir->st.id.mode & BT_DIR)
	sprintf(buf,"%5s","DIR");
      else {
	i = bt_rlen(dir->dir->lbuf,dir->dir->rlen);
	if (i)
	  sprintf(buf,"%5d",i);
	else
	  sprintf(buf,"%5s","");
      }
      break;
    case dirKlen:
      i = bt_klen(dir->dir->lbuf);
      if (i == 0 || (dir->st.id.mode & BT_DIR))
	sprintf(buf,"%5s","---");
      else
	sprintf(buf,"%5d",i);
      break;
    }
  }
}

static int DirField_store(Column *c,sql x,char *buf) {
  DirField *this = (DirField *)c;
  /* can't modify directory at present */
  /* FIXME: support modifying certain fields:
     dirName, dirUid, dirGid, dirUser, dirGroup, dirPerm, dirMtime, etc.
  */
  switch (this->fld) {
  case dirName:
    if (x->op == EXSTRING) {
      stchar(x->u.name[0],buf,this->len);
      return 0;
    }
    break;
  }
  return -1;
}

static sql DirField_load(Column *c) {
  DirField *this = (DirField *)c;
  Directory *dir = this->dir;
  sql x;
  const char *s;
  if (this->fld != dirName)
    Directory_stat(dir);
  switch (this->fld) {
  case dirName:
    x = mksql(EXSTRING);
    x->u.name[0] = obstack_copy0(sqltree,dir->dir->lbuf,strlen(dir->dir->lbuf));
    break;
  case dirMid: case dirUid: case dirGid: case dirMode:
  case dirLinks: case dirLocks:
    x = mksql(EXCONST);
    x->u.num.val = ltoM((long)*(short *)this->buf);
    x->u.num.fix = 0;
    break;
  case dirUser:
    x = mksql(EXSTRING);
    s = pwName(dir->st.id.user);
    x->u.name[0] = obstack_copy0(sqltree,s,strlen(s));
    break;
  case dirGroup:
    x = mksql(EXSTRING);
    s = pwName(dir->st.id.group);
    x->u.name[0] = obstack_copy0(sqltree,s,strlen(s));
    break;
  case dirPerm:
    x = mksql(EXSTRING);
    s = permstr(dir->st.id.mode);
    x->u.name[0] = obstack_copy0(sqltree,s,strlen(s));
    break;
  case dirRoot: case dirBlks: case dirRecs:
  case dirMtime: case dirAtime: case dirCtime:
    x = mksql(EXCONST);
    x->u.num.val = ltoM(*(long *)this->buf);
    x->u.num.fix = 0;
    break;
  case dirRlen:
    x = mksql(EXCONST);
    if (dir->st.id.mode & BT_DIR)
      x->u.num.val = zeroM;
    else
      x->u.num.val = ltoM(bt_rlen(dir->dir->lbuf,dir->dir->rlen));
    x->u.num.fix = 0;
    break;
  case dirKlen:
    x = mksql(EXCONST);
    if (dir->st.id.mode & BT_DIR)
      x->u.num.val = zeroM;
    else
      x->u.num.val = ltoM(bt_klen(dir->dir->lbuf));
    x->u.num.fix = 0;
    break;
  }
  return x;
}

static Column *DirField_dup(Column *c,char *buf) {
  DirField *this = (DirField *)c;
  if (buf == this->buf)
    return Column_dup(c,buf);
  switch (this->fld) {
  case dirName: case dirUser: case dirGroup: case dirPerm:
    c = Charfld_init(0,buf,this->len); break;
  case dirRoot:
    c = Column_init(0,buf,this->len); break;
  case dirMid:
    c = Number_init_mask(0,buf,this->len,"Z#"); break;
  case dirBlks: case dirRecs:
    c = Number_init_mask(0,buf,this->len,"ZZZZZZZ#"); break;
  case dirUid: case dirGid: case dirMode:
  case dirLinks: case dirLocks:
    c = Number_init_mask(0,buf,this->len,"ZZZZ#"); break;
  case dirRlen: case dirKlen:
    c = Number_init_mask(0,buf,this->len,"ZZZZZ"); break;
  case dirMtime: case dirAtime: case dirCtime:
    c = Time_init_mask(0,buf,ftmask); break;
  }
  Column_name(c,this->name,this->desc);
  return c;
}

static void DirField_copy(Column *c,char *buf) {
  DirField *this = (DirField *)c;
  Directory *dir = this->dir;
  if (this->fld != dirName)
    Directory_stat(dir);
  switch (this->fld) {
  case dirName:
    stchar(dir->dir->lbuf,buf,this->len); break;
  case dirRoot: case dirBlks: case dirRecs:
  case dirMtime: case dirAtime: case dirCtime:
    stlong(*(t_block *)this->buf,buf); break;
  case dirMid: case dirUid: case dirGid: case dirMode:
  case dirLinks: case dirLocks:
    stshort(*(short *)this->buf,buf); break;
  case dirUser:
    stchar(pwName(dir->st.id.user),buf,this->len); break;
  case dirGroup:
    stchar(grName(dir->st.id.group),buf,this->len); break;
  case dirPerm:
    stchar(permstr(dir->st.id.mode),buf,this->len); break;
  case dirRlen:
    if (dir->st.id.mode & BT_DIR)
      stshort(0,buf);
    else
      stshort(bt_rlen(dir->dir->lbuf,dir->dir->rlen),buf); break;
  case dirKlen:
    if (dir->st.id.mode & BT_DIR)
      stshort(0,buf);
    else
      stshort(bt_klen(dir->dir->lbuf),buf); break;
  }
}

static struct Column_class DirField_class = {
  sizeof (DirField), Column_free, DirField_print, DirField_store, DirField_load,
  DirField_copy, DirField_dup
};

static Column *DirField_init(DirField *this,Directory *dir,int what) {
  if (!this && !(this = malloc(sizeof *this))) return 0;
  Column_init((Column *)this,0,0);
  this->_class = &DirField_class;
  this->dir = dir;
  this->fld = what;
  this->buf = (char *)dir + dirTable[what].offset;
  this->width = this->dlen = dirTable[what].width;
  this->len = dirTable[what].len;
  this->type = dirTable[what].type;
  this->name = dirTable[what].name;
  this->desc = dirTable[what].desc;
  return (Column *)this;
}

static int Directory_find(Cursor *c);
static int Directory_next(Cursor *c);
static int Directory_first(Cursor *c);
static void Directory_free(Cursor *c);

static struct Cursor_class directory_class = {
  sizeof (Cursor),
  Directory_free,
  Directory_first,Directory_next,Directory_find,
  Cursor_print,
	Cursor_fail,Cursor_fail,Cursor_fail,Cursor_optim
};

Cursor *Directory_init(const char *name,int rdonly) {
  Directory *this = xmalloc(sizeof *this);
  int i;
  int rc;
  rdonly = 1;
  this->col = 0;
  this->dir = 0;
  catch (rc)
    this->_class = &directory_class;
    this->dir = btopen(name,
	(rdonly ? BTRDONLY + BTDIROK : BTRDWR + BTDIROK) | NOKEY,MAXREC);
    if (this->dir->flags == 0)
      errpost(BTERKEY);
    //this->rows = btfstat(this->dir,&this->st) ? 0 : this->st.rcnt;
    this->rows = 0;
    this->cnt = 0;
    this->klen = 1;
    this->ncol = dirNcols;	/* this many implemented */
    this->col = xmalloc(this->ncol * sizeof *this->col);
    for (i = 0; i < this->ncol; ++i)
      this->col[i] = DirField_init(this->dcols + i,this,i);
  enverr
    switch (rc) {
    case BTERKEY:
      /* fprintf(stderr,"%s not found\n",name); */
      break;
    default:
      fprintf(stderr,"Directory_init error %d\n",rc);
    }
    Directory_free((Cursor *)this);
    return 0;
  envend
  return (Cursor *)this;
}

static void Directory_free(Cursor *c) {
  Directory *this = (Directory *)c;
  btclose(this->dir);
  if (this->col)
    free(this->col);
  free(this);
}

static int Directory_first(Cursor *c) {
  Directory *this = (Directory *)c;
  this->dir->rlen = MAXREC;
  this->dir->klen = 0;
  if (btas(this->dir,BTREADGE + NOKEY)) {
    this->rows = this->cnt = 0;
    return -1;
  }
  stchar(this->dir->lbuf,this->namekey,sizeof this->namekey);
  this->cnt = 1;
  this->stat = 0;
  return 0;
}

static int Directory_next(Cursor *c) {
  Directory *this = (Directory *)c;
  this->dir->klen = this->dir->rlen;
  this->dir->rlen = MAXREC;
  if (btas(this->dir,BTREADGT + NOKEY)) {
    this->rows = this->cnt;
    return -1;
  }
  stchar(this->dir->lbuf,this->namekey,sizeof this->namekey);
  ++this->cnt;
  this->stat = 0;
  return 0;
}

static int Directory_find(Cursor *c) {
  Directory *this = (Directory *)c;
  this->dir->rlen = MAXREC;
  ldchar(this->namekey,sizeof this->namekey,this->dir->lbuf);
  this->dir->klen = strlen(this->dir->lbuf) + 1;
  if (btas(this->dir,BTREADGE + NOKEY)) {
    this->rows = this->cnt = 0;
    return -1;
  }
  stchar(this->dir->lbuf,this->namekey,sizeof this->namekey);
  this->cnt = 1;
  this->stat = 0;
  return 0;
}
