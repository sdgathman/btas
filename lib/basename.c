#include <string.h>
#include <btas.h>

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
