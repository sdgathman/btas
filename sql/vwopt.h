#include "sql.h"

typedef struct vwsel {
  char *min, *max;
  sql exp;		/* must also be true for selected records */
  struct vwsel *next;
} Vwsel;

struct vwsel_ctx;
typedef struct vwsel_ctx Vwsel_ctx;
/* start new vwsel context */
Vwsel_ctx *Vwsel_push(Vwsel_ctx *,struct Column **, int, int);
void Vwsel_free_ctx(Vwsel_ctx *);

Vwsel *Vwsel_and_set(Vwsel *, const Vwsel *);
Vwsel *Vwsel_or_set(Vwsel *, const Vwsel *);
Vwsel *Vwsel_new(void);
Vwsel *Vwsel_copy(const Vwsel *);
Vwsel *Vwsel_init(sql x);
long Vwsel_estim(const Vwsel *, long);
void Vwsel_free(Vwsel *);
void Vwsel_print(const Vwsel *);
void inc(char *, int);
void dec(char *, int);
