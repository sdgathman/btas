#include "VHMap.h"
/* codes for status fields */

template <class T,class C>
VHMap<T,C>::VHMap(const C& dflt,unsigned sz): Map<T,C>(dflt) {
  tab = new T[size = sz];
  cont = new C[size];
  status = new char[size];
  for (unsigned int i = 0; i < size; ++i)
    status[i] = EMPTYCELL;
}

template <class T,class C>
VHMap<T,C>::VHMap(VHMap& a): Map<T,C>(a.def) {
  tab = new T[size = a.size];
  cont = new C[size];
  status = new char[size];
  for (unsigned int i = 0; i < size; ++i) status[i] = EMPTYCELL;
  count = 0;
  for (Pix p = a.first(); p; a.next(p)) (*this)[a.key(p)] = a.contents(p);
}


/* 
 * hashing method: double hash based on high bits of hash fct,
 * followed by linear probe. Can't do too much better if table
 * sizes not constrained to be prime.
*/


static inline unsigned int Mapdoublehashinc(unsigned int h, unsigned int s) {
  unsigned int dh =  ((h / s) % s);
  return (dh > 1)? dh : 1;
}


template <class T,class C>
void VHMap<T,C>::resize(unsigned newsize) {
  if (newsize <= count)
  {
    newsize = 100;
    while (newsize <= count) newsize <<= 1;
  }
  T* oldtab = tab;
  C* oldcont = cont;
  char* oldstatus = status;
  unsigned int oldsize = size;
  tab = new T[size = newsize];
  cont = new C[size];
  status = new char[size];
  unsigned int i;
  for (i = 0; i < size; ++i) status[i] = EMPTYCELL;
  count = 0;
  for (i = 0; i < oldsize; ++i) 
    if (oldstatus[i] == VALIDCELL) 
      (*this)[oldtab[i]] = oldcont[i];
  delete [] oldtab;
  delete [] oldcont;
  delete oldstatus;
}

template <class T,class C>
VHMap<T,C>::~VHMap() {
  delete [] tab;
  delete [] cont;
  delete [] status;
}

template <class T,class C>
bool VHMap<T,C>::OK() const {
  bool v = tab != 0;
  v &= status != 0;
  int n = 0;
  for (unsigned int i = 0; i < size; ++i) 
  {
    if (status[i] == VALIDCELL) ++n;
    else if (status[i] != DELETEDCELL && status[i] != EMPTYCELL)
      v = false;
  }
  v &= n == count;
  if (!v) error("invariant failure");
  return v;
}

template <class T,class C>
Pix VHMap<T,C>::seek(const T& key) const {
  unsigned int hashval = hash(key);
  unsigned int h = hashval % size;
  for (unsigned int i = 0; i <= size; ++i)
  {
    if (status[h] == EMPTYCELL)
      return 0;
    else if (status[h] == VALIDCELL && key == tab[h])
      return Pix(&tab[h]);
    if (i == 0)
      h = (h + Mapdoublehashinc(hashval, size)) % size;
    else if (++h >= size)
      h -= size;
  }
  return 0;
}

template <class T,class C>
C& VHMap<T,C>::operator [] (const T& item) {
  if (size <= count + 1)
    resize(0);

  unsigned int bestspot = size;
  unsigned int hashval = hash(item);
  unsigned int h = hashval % size;
  for (unsigned int i = 0; i <= size; ++i)
  {
    if (status[h] == EMPTYCELL)
    {
      ++count;
      if (bestspot >= size) bestspot = h;
      tab[bestspot] = item;
      status[bestspot] = VALIDCELL;
      cont[bestspot] = def;
      return cont[bestspot];
    }
    else if (status[h] == DELETEDCELL)
    {
      if (bestspot >= size) bestspot = h;
    }
    else if (tab[h] == item)
      return cont[h];

    if (i == 0)
      h = (h + Mapdoublehashinc(hashval, size)) % size;
    else if (++h >= size)
      h -= size;
  }

  ++count;
  status[bestspot] = VALIDCELL;
  tab[bestspot] = item;
  cont[bestspot] = def;
  return cont[bestspot];
}

template <class T,class C>
void VHMap<T,C>::del(const T& key) {
  unsigned int hashval = hash(key);
  unsigned int h = hashval % size;
  for (unsigned int i = 0; i <= size; ++i)
  {
    if (status[h] == EMPTYCELL)
      return;
    else if (status[h] == VALIDCELL && key == tab[h])
    {
      status[h] = DELETEDCELL;
      --count;
      return;
    }
    if (i == 0)
      h = (h + Mapdoublehashinc(hashval, size)) % size;
    else if (++h >= size)
      h -= size;
  }
}

template <class T,class C>
void VHMap<T,C>::next(Pix& i) const {
  if (i == 0) return;
  unsigned int pos = ((unsigned)i - (unsigned)tab) / sizeof(T) + 1;
  for (; pos < size; ++pos)
    if (status[pos] == VALIDCELL)
    {
      i = Pix(&tab[pos]);
      return;
    }
  i = 0;
}

template <class T,class C>
Pix VHMap<T,C>::first() const {
  for (unsigned int pos = 0; pos < size; ++pos)
    if (status[pos] == VALIDCELL) return Pix(&tab[pos]);
  return 0;
}

template <class T,class C>
void VHMap<T,C>::clear() {
  for (unsigned int i = 0; i < size; ++i) status[i] = EMPTYCELL;
  count = 0;
}
