/*
	SQL pattern match
*/

#include "config.h"
#include "sql.h"

int like(p,pat)
  const char *p;	/* string to match */
  const char *pat;	/* pattern */
{
  while (*pat) {
    switch (*pat) {
    case '_':
      if (*p == 0) return 0;
      ++pat;
      break;		/* always matches */
    case '%':		/* match any number of characters */
      if (!*++pat) return 1;
      do 
	if (like(p,pat)) return 1;
      while (*++p);
      return 0;
#ifdef RANGE
    case '[':
    { char inverse;
      if (*++pat != '!' || pat[1] == ']')
	inverse = 0;
      else {
	inverse = 1;
	++pat;
      }
      while (*pat) {
	if (
	  pat[1] == '-' && *pat++ <= *p && *p <= *++pat	/* range */
	  || *p == *pat					/* single */
	) {
	  if (inverse) return 0;
	  inverse = 1;
	}
	if (!*pat) break;
	if (*++pat == ']') {
	  ++pat;
	  break;
	}
      }
      if (!inverse) return 0;
      break;
    }
#endif
    default:
      if (*p != *pat++) return 0;
    }
    ++p;
  }
  if (*p) return 0;
  return 1;		/* matches */
}
