/*	interface C sql code to a C++ object
*/
#include <iostream.h>
#include <Cursor.h>
extern "C" {
#define delete del
#include "cursor.h"
#include "object.h"
#undef delete

void *xmalloc(unsigned size) {
  void *p = malloc(size);
  if (!p && size) {
    cerr << "Out of memory!" << endl;
    /* abort();	/* let debugger show us how we used it up */
    exit(1);	/* tricky to close files, etc. with objects */
		/* need catch, throw */
  }
  return p;
}

int debug = 0;
int mapupper = 0;
volatile int cancel = 0;

void sql_syntax(const char *s,const char *p,const char *lexptr) {
}

}

class SqlColumn: public DColumn {
  Column *col;
public:
  SqlColumn &operator =(Column *);
  operator String() const;
  DColumn &operator=(const String &);
};

SqlColumn &SqlColumn::operator =(Column *c) {
  col = c;
  name = c->name;
  desc = c->desc;
  dlen = c->width;
  return *this;
};

SqlColumn::operator String() const {
  char *buf = (char *)alloca(dlen);
  do2(col,print,VALUE,buf);
  return String(buf,dlen);
}

DColumn &SqlColumn::operator=(const String &) {
  // do nothing yet
  return *this;
}

SqlCursor::SqlCursor(const String &s): cur(0), cola(0) {
  h = new obstack;
  obstack_init(h);
  free_sql(h);
  String exp = s + ';';
  sql_stmt *cmd = parse_sql(exp);
  if (cmd) {
    switch (cmd->cmd) {
    case SQL_SELECT:
      if (cur = Select_init(cmd->src,1,0)) {
	// setup array of Columns
	cola = new SqlColumn[cur->ncol];
	col  = new DColumn *[cur->ncol];
	ncol = cur->ncol;
	klen = cur->klen;
	for (int i = 0; i < ncol; ++i) {
	  cola[i] = cur->col[i];
	  col[i] = &cola[i];
	}
      }
      else
	cerr << "Select_init failed" << endl;
      break;
    }
  }
}

SqlCursor::~SqlCursor() {
  delete [] cola;
  delete [] col;
  if (cur)
    do0(cur,free);
  obstack_free(h,0);
  delete h;
}

int SqlCursor::first(int n) {
  if (!cur || do0(cur,first)) return 0;
  return next(n) + 1;
}

int SqlCursor::next(int n) {
  if (!cur) return 0;
  int cnt;
  for (cnt = 0; cnt < n; ++cnt)
    if (do0(cur,next)) break;
  return cnt;
}

int SqlCursor::find() {
  if (!cur) return 0;
  return do0(cur,find);
}
