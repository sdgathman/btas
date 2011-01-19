/* isam.h additions for BTAS/X C-isam emulator */
#ifndef k_start
#ifdef __cplusplus
extern "C" {
#endif

#ifndef MORE
#include <port.h>
#endif

enum {
  CHARTYPE=0, DECIMALTYPE=0,INTTYPE,LONGTYPE,DOUBLETYPE,FLOATTYPE,MAXTYPE,
  ISDESC=0x80,	/* add to make descending type */
  TYPEMASK=0x7F,/* type mask			*/

  ISNODUPS  = 000,	/* no duplicates allowed	*/
  ISDUPS    = 001,	/* duplicates allowed		*/
  DCOMPRESS = 002,	/* duplicate compression	*/
  LCOMPRESS = 004,	/* leading compression		*/
  TCOMPRESS = 010,	/* trailing compression		*/
  COMPRESS  = 016,	/* all compression		*/
  ISCLUSTER = 020,	/* index is a cluster one       */

  CHARSIZE	= 1, INTSIZE	= 2, LONGSIZE	= 4,
  FLOATSIZE	= 4, DOUBLESIZE	= 8
};

#if 0
#define USERCOLL(x)	((x))
enum {
  COLLATE1=0x10,
  COLLATE2=0x20,
  COLLATE3=0x30,
  COLLATE4=0x40,
  COLLATE5=0x50,
  COLLATE6=0x60,
  COLLATE7=0x70
};
#endif

#ifdef BBN			/* BBN Machine has 10 bits/byte	*/
enum { BYTEMASK=0x3FF,BYTESHFT=10 };
#else
enum { BYTEMASK=0xFF,BYTESHFT=8 };
#endif

#ifndef	ldint
#define ldint(p)	((short)(((p)[0]<<BYTESHFT)+((p)[1]&BYTEMASK)))
#define stint(i,p)	((p)[0]=(i)>>BYTESHFT,(p)[1]=(i))
#endif

#if 0
long ldlong(const char *);
void stlong(long,char *);
#endif

float	ldfloat(const char *);
double	lddbl(const char *);
/* double ldfltnull(); */
/* double lddblnull(); */

enum isread_op {
  ISFIRST,	/* position to first record	*/
  ISLAST,	/* position to last record	*/
  ISNEXT,	/* position to next record	*/
  ISPREV,	/* position to previous record	*/
  ISCURR,	/* position to current record	*/
  ISEQUAL,	/* position to equal value	*/
  ISGREAT,	/* position to greater value	*/
  ISGTEQ,	/* position to >= value		*/
  ISLESS,	/* position to < value		*/
  ISLTEQ,	/* position to >= value 	*/
  /* isread lock modes */
  ISLOCK = 0x100,	/* record lock */
  ISWAIT = 0x400,	/* wait for record lock */
  ISFULL = 0x800,	/* use full key for ISGREAT,ISLESS */
  ISLCKW = ISLOCK+ISWAIT
};

enum isopen_op {
  ISINPUT = 0,	/* open for input only		*/
  ISOUTPUT = 1,	/* open for output only		*/
  ISINOUT = 2,	/* open for input and output	*/
  ISTRANS = 4,	/* open for transaction proc	*/
/* isopen, isbuild lock modes */
  ISDIROK    = 0x100,	/* allow directory open */
  ISAUTOLOCK = 0x200,	/* automatic record lock	*/
  ISMANULOCK = 0x400,	/* manual record lock		*/
  ISEXCLLOCK = 0x800,	/* exclusive isam file lock	*/
/* BMS extended open modes */
  READONLY   = ISMANULOCK+ISINPUT,
  UPDATE     = ISMANULOCK+ISINOUT
};

/* audit trail mode parameters */
enum isaudit_op {
  AUDSETNAME	= 0,	/* set new audit trail name	*/
  AUDGETNAME	= 1,	/* get audit trail name		*/
  AUDSTART	= 2,	/* start audit trail 		*/
  AUDSTOP	= 3,	/* stop audit trail 		*/
  AUDINFO	= 4,	/* audit trail running ?	*/
};

enum {
  MAXFDS = 63,
  MAXKEYSIZE = 120,	/* max number of bytes in key	*/
  NPARTS = 8		/* max number of key parts	*/
};

struct keypart {
  short kp_start;		/* starting byte of key part	*/
  short kp_leng;		/* length in bytes		*/
  short kp_type;		/* type of key part		*/
};

struct keydesc {
  short k_flags;		/* flags			*/
  short k_nparts;		/* number of parts in key	*/
  struct keypart k_part[NPARTS];	/* each key part	*/
		  /* the following is for internal use only	*/
  short k_len;		/* length of whole key		*/
  long k_rootnode;		/* pointer to rootnode		*/
};

#define k_start   k_part[0].kp_start
#define k_leng    k_part[0].kp_leng
#define k_type    k_part[0].kp_type

struct dictinfo {
  short di_nkeys;		/* number of keys defined	*/
  short di_recsize;		/* data record size		*/
  short di_idxsize;		/* index record size		*/
  long di_nrecords;		/* number of records in file	*/
};

enum isam_err {
  EDUPL	   =100,	/* duplicate record	*/
  ENOTOPEN =101,	/* file not open	*/
  EBADARG  =102,	/* illegal argument	*/
  EBADKEY  =103,	/* illegal key desc	*/
  ETOOMANY =104,	/* too many files open	*/
  EBADFILE =105,	/* bad isam file format	*/
  ENOTEXCL =106,	/* non-exclusive access	*/
  ELOCKED  =107,	/* record locked	*/
  EKEXISTS =108,	/* key already exists	*/
  EPRIMKEY =109,	/* is primary key	*/
  EENDFILE =110,	/* end/begin of file	*/
  ENOREC   =111,	/* no record found	*/
  ENOCURR  =112,	/* no current record	*/
  EFLOCKED =113,	/* file locked		*/
  EFNAME   =114,	/* file name too long	*/
  ENOLOK   =115,	/* can't create lock file */
  EBADMEM  =116,	/* can't alloc memory	*/
  EBADCOLL =117,	/* bad custom collating	*/
  ELOGREAD =118,	/* cannot read log rec  */
  EBADLOG  =119,	/* bad log record	*/
  ELOGOPEN =120,	/* cannot open log file	*/
  ELOGWRIT =121,	/* cannot write log rec */
  ENOTRANS =122,	/* no transaction	*/
  ENOSHMEM =123,	/* no shared memory	*/
  ENOBEGIN =124,	/* no begin work yet	*/
  ENONFS   =125		/* can't use nfs */
};

/*
 * For system call errors
 *   iserrno = errno (system error code 1-99)
 *   iserrio = IO_call + IO_file
 *		IO_call  = what system call
 *		IO_file  = which file caused error
 */

enum {
  IO_OPEN	 =0x10,		/* open()	*/
  IO_CREA	 =0x20,		/* creat()	*/
  IO_SEEK	 =0x30,		/* lseek()	*/
  IO_READ	 =0x40,		/* read()	*/
  IO_WRIT	 =0x50,		/* write()	*/
  IO_LOCK	 =0x60,		/* locking()	*/
  IO_IOCTL  	 =0x70,		/* ioctl()	*/

  IO_IDX	 =0x01,		/* index file	*/
  IO_DAT	 =0x02,		/* data file	*/
  IO_AUD	 =0x03,		/* audit file	*/
  IO_LOK	 =0x04,		/* lock file	*/
  IO_SEM	 =0x05		/* semaphore file */
};

extern int iserrno;		/* isam error return code	*/
extern int iserrio;		/* system call error code	*/
extern long isrecnum;		/* record number of last call	*/
extern char isstat1;		/* cobol status characters	*/
extern char isstat2;
extern char *isversnumber;	/* C-ISAM version number	*/
extern char *iscopyright;	/* RDS copyright		*/
extern char *isserial;		/* C-ISAM software serial number */
extern int  issingleuser;	/* set for single user access	*/
extern int  is_nerr;		/* highest C-ISAM error code	*/
extern const char *is_errlist[];	/* C-ISAM error messages	*/
/*  error message usage:
 *	if (iserrno >= 100 && iserrno < is_nerr)
 *	    printf("ISAM error %d: %s\n", iserrno, is_errlist[iserrno-100]);
 */

struct audhead {
  char au_type[2];		/* audit record type aa,dd,rr,ww*/
  char au_time[4];		/* audit date-time		*/
  char au_procid[2];		/* process id number		*/
  char au_userid[2];		/* user id number		*/
  char au_recnum[4];		/* record number		*/
};

enum { AUDHEADSIZE = 14 };	/* num of bytes in audit header */

void bld(const char *,int,const struct keydesc *,int);
void bldx(const char *,const char *,int,int,const struct keydesc *,int);
int vwsel(int,const char *,const char *,char *,const struct keydesc *);
int vwrev(int,const char *,const char *,char *,const struct keydesc *);
int circ_read(int,char *,int);
int circ_sel(int,char *,const char *,const char *,struct keydesc *,int);
void closefls(void);

#define VWSEL(f,n,x,b,k) vwsel(f,(char *)n,(char *)x,(char *)b,k)
#define VWREV(f,n,x,b,k) vwrev(f,(char *)n,(char *)x,(char *)b,k)
#define CIRC_SEL(f,b,n,x,k,d) circ_sel(f,(char *)b,(char *)n,(char *)x,k,d)
#define closefls iscloseall

/* currently implemented functions */

int isopen(const char *, int);
int isopenx(const char *,int,int);
int isbuild(const char *,int,const struct keydesc *,int);
struct btflds;
struct btfrec;
int isbuildx(const char *,int,const struct keydesc *,int ,
		    struct btflds *);
struct btflds *isflds(int fd);
int isaddflds(int fd,const struct btfrec *flds,int cnt);
int isclose(int);
int isread(int, PTR, int);
int isrange(int, const PTR, const PTR);
int isstart(int, const struct keydesc *, int, const PTR, int);
int isstartn(int, const char *, int, const PTR, int);
int iswrite(int, const PTR);
int iswrcurr(int, const PTR);
int isrewrite(int, const PTR);
int isrewcurr(int, const PTR);
int isrewrec(int, long, const PTR);
int isdelete(int, const PTR);
int isdelcurr(int);
int isdelrec(int, long);
int isuniqueid(int, long *);
int issetunique(int, long);
int isbtasinfo(int, struct btstat *);
int isindexinfo(int, struct keydesc *, int);
int isindexname(int, char *buf, int);	/* buf should be MAXKEYSIZE chars */
int isreclen(int);
int isaddindex(int, const struct keydesc *);
int isaddindexn(int, const struct keydesc *,const char *);
int isdelindex(int, const struct keydesc *);
int isfixindex(int, const struct keydesc *);
int isflush(int);	/* flush dirty buffers */
int isrelease(int);	/* release all locked records */
int iserase(const char *);
int isrename(const char *, const char *);
int islock(int), isunlock(int);
int islockx(int);
void iscloseall(void);

#ifdef __cplusplus
}
#endif
#endif
