#include <port.h>
#include <stdlib.h>
extern PTR xmalloc(/**/ unsigned /**/);
#define obstack_chunk_alloc	xmalloc
#define obstack_chunk_free	free
#include "obstack.h"
