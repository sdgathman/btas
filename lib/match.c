/*
	Filename pattern match
*/

#include <btas.h>

int match(const char *p,const char *pat) {
  register char inverse;
  while (*pat) {
    switch (*pat) {
    case 0:
      return 0;		/* too short */
    case '?':
      ++pat;
      break;		/* always matches */
    case '*':		/* match any number of characters */
      if (!*++pat) return 1;
      do 
	if (match(p,pat)) return 1;
      while (*++p);
      return 0;
    case '[':
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
    default:
      if (*p != *pat++) return 0;
    }
    ++p;
  }
  if (*p) return 0;
  return 1;		/* matches */
}
