#pragma interface
#include "Map.h"

template <class T,class C>
Pix Map<T,C>::seek(const T& item) const {
  Pix i;
  for (i = first(); i != 0 && !(key(i) == item); next(i));
  return i;
}

template <class T,class C>
void Map<T,C>::clear() {
  Pix i = first(); 
  while (i != 0)
  {
    del(key(i));
    i = first();
  }
}

template <class T,class C>
bool Map<T,C>::contains (const T& item) const {
  return seek(item) != 0;
}

template <class T,class C>
void Map<T,C>::error(const char* msg) const {
  (*lib_error_handler)("Map", msg);
}
