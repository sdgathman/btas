#include "sql.h"

struct btfrec;

enum Column_type { TITLE, SCORE, VALUE, DATA };

typedef struct Column Column;

struct Column_class {
  int size;
  void (*free)	(Column *);
  void (*print)(Column *, enum Column_type, char *);
  int (*store)  (Column *, sql, char *);	/* sql to buffer */
  sql  (*load)  (Column *);		/* buffer to sql */
  void (*copy)  (Column *, char *);	/* copy data to buffer */
  Column *(*dup)(Column *, char *);	/* dup col w/ new buffer */
};

struct Column {
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
};

typedef struct Number Number;

Column *Column_gen(char *, const struct btfrec *, char *);
Column *Column_sql(sql, char *);
Column *Column_init(Column *, char *, int);
Column *Number_init(Number *, char *, int, int);
Column *Pnum_init(Number *, char *, int, const char *);
Column *Number_init_mask(Number *, char *, int, const char *);
Column *Charfld_init(Column *, char *, int);
Column *Ebcdic_init(Column *, char *, int);
Column *Dblfld_init(Column *, char *);
void Column_free(Column *);
void Column_print(Column *, enum Column_type, char *);
void Column_name(Column *, const char *, const char *);
void Column_copy(Column *, char *);
Column *Column_dup(Column *, char *);
void printColumn(Column *, enum Column_type);
