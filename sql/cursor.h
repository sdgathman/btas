#include "column.h"

typedef struct Cursor Cursor;

struct Cursor_class {
  int size;
  void (*free)	(Cursor *);
  /* traversing cursor records */
  int (*first)	(Cursor *);
  int (*next)	(Cursor *);
  /* find the first record ge the key columns */
  int (*find)	(Cursor *);
  void (*print) (Cursor *, enum Column_type, const char * );
  /* updating cursor */
  int (*insert) (Cursor *);
  int (*update) (Cursor *);
  int (*del) (Cursor *);
  /* feed back key and data columns actually used for optimization */
  void (*optim) (Cursor *, Column **, int, int);
  /* required for proper updating */
  int (*lock) (Cursor *);	/* lock current record for updating */
  /* functions for table expressions */
  Cursor *(*join) (Cursor *,sql where,Cursor *);
};

struct Cursor {
  struct Cursor_class *_class;
/* public: */
  short ncol;	/* number of columns */
  short klen;	/* number of key columns */
  Column **col;
  long rows;	/* estimated number of rows */
  long cnt;	/* actual returned so far */
};

/* ??table.c */
Cursor *Table_init(const char *,int);
int Isam_getfd(Cursor *);
Cursor *Directory_init(const char *,int);
struct btflds;
void getdict(Cursor *, const char *, char *, const struct btflds *);
/* needed for method tables */
void Cursor_print(Cursor *, enum Column_type, const char *);
void Cursor_optim(Cursor *, Column **, int, int);
int Cursor_fail(Cursor *);
/* join.c */
Cursor *Join_init(int, Cursor **);
/* vwsel.c */
Cursor *Where_init(Cursor *, sql, int, int);
/* exec.c */
struct tabinfo;
Cursor *Select_init(const struct select *, int, struct tabinfo *);
/* values.c */
Cursor *Values_init(sql);
/* order.c */
Cursor *Order_init(Cursor *, sql, sql, int,int);
Cursor *Group_init(Cursor *, sql, sql, sql,int);
Cursor *Selcol_init(Cursor *, sql);

extern Cursor System_table;

/* cause immediate EOF on all cursors */
extern volatile int cancel;
extern volatile int inexec;
