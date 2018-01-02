/*
    This file is part of the BTAS client library.

    The BTAS client library is free software: you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of the License,
    or (at your option) any later version.

    BTAS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with BTAS.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <btas.h>
#include <string.h>

/** Filename pattern match.
 * @param p	filename to test for match
 * @param pat	pattern to match
 * @return 1 for match, 0 for no match
 */
int match(const char *p,const char *pat) {
  char inverse;
  const char *b = p;
  while (*pat) {
    switch (*pat) {
    case 0:
      return 0;		/* too short */
    case '?':
      ++pat;
      break;		/* always matches */
    case '*':		/* match any number of characters */
      if (!*++pat) return 1;
      if (strchr("*?[",*pat)) {
	do 
	  if (match(p,pat)) return p - b + 1;
	while (*p++);
      }
      else {
	while ((p = strchr(p,*pat))) {
	  if (match(p++,pat)) return p - b;
	}
      }
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
	  (pat[1] == '-' && *pat++ <= *p && *p <= *++pat)	/* range */
	  || *p == *pat						/* single */
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

#if 0
#include <stdio.h>

int main(int argc,char **argv) {
  char buf[256];
  while (gets(buf)) {
    if (match(buf,argv[1]))
      puts(buf);
  }
  return 0;
}
#endif
