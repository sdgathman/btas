#include <string.h>
#include <btas.h>

#ifndef _GNU_SOURCE
/* If we don't have the GNU basename(), we need to define a compatible one
  (that never modifies its argument). */
char *basename(const char *name) {
  char const *fname = strrchr(name,'/');
  return (char *)(fname ? ++fname : name);
}

char *dirname(char *name) {
  char *fname = strrchr(name,'/');
  if (fname == 0) return "";
  if (fname == name) return "/";
  *fname = 0;
  return name;
}
#endif
