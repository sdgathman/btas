/* 
 * hashing method: double hash based on high bits of hash fct,
 * followed by linear probe. Can't do too much better if table
 * sizes not constrained to be prime.
*/

static inline unsigned int doublehashinc(unsigned int h, unsigned int s) {
  unsigned int dh =  ((h / s) % s);
  return (dh > 1)? dh : 1;
}

template <class T>
VHSet<T>::VHSet(unsigned int sz) {
  tab = new T[size = sz];
  status = new char[size];
  for (unsigned int i = 0; i < size; ++i) status[i] = EMPTYCELL;
  count = 0;
}

template <class T>
VHSet<T>::VHSet(const Set<T>& a) {
  tab = new T[size = a.length() * 2];
  status = new char[size];
  for (unsigned int i = 0; i < size; ++i) status[i] = EMPTYCELL;
  count = 0;
  for (Pix p = a.first(); p; a.next(p)) add(a(p));
}

template <class T>
void VHSet<T>::resize(unsigned int newsize) {
  if (newsize <= count) {
    newsize = 100;
    while (tableTooCrowded(count, newsize))  newsize <<= 1;
  }
  T* oldtab = tab;
  char* oldstatus = status;
  unsigned int oldsize = size;
  tab = new T[size = newsize];
  status = new char[size];
  unsigned i;
  for (i = 0; i < size; ++i)
    status[i] = EMPTYCELL;
  count = 0;
  for (i = 0; i < oldsize; ++i)
    if (oldstatus[i] == VALIDCELL) add(oldtab[i]);
  delete [] oldtab;
  delete oldstatus;
}

template <class T>
int VHSet<T>:: operator == (const VHSet& b) const {
  if (count != b.count)
    return 0;
  else {
    unsigned i;
    for (i = 0; i < size; ++i)
      if (status[i] == VALIDCELL && b.seek(tab[i]) == 0)
          return 0;
    for (i = 0; i < b.size; ++i)
      if (b.status[i] == VALIDCELL && seek(b.tab[i]) == 0)
          return 0;
    return 1;
  }
}

template <class T>
int VHSet<T>::operator <= (const VHSet& b) const {
  if (count > b.count)
    return 0;
  else {
    for (unsigned int i = 0; i < size; ++i)
      if (status[i] == VALIDCELL && b.seek(tab[i]) == 0)
          return 0;
    return 1;
  }
}

template <class T>
void VHSet<T>::operator |= (const VHSet& b) {
  if (&b == this || b.count == 0)
    return;
  for (unsigned int i = 0; i < b.size; ++i)
    if (b.status[i] == VALIDCELL) add(b.tab[i]);
}

template <class T>
void VHSet<T>::operator &= (const VHSet& b) {
  if (&b == this || count == 0)
    return;
  for (unsigned int i = 0; i < size; ++i) {
    if (status[i] == VALIDCELL && b.seek(tab[i]) == 0) {
      status[i] = DELETEDCELL;
      --count;
    }
  }
}

template <class T>
void VHSet<T>::operator -= (const VHSet& b) {
  for (unsigned int i = 0; i < size; ++i) {
    if (status[i] == VALIDCELL && b.seek(tab[i]) != 0) {
      status[i] = DELETEDCELL;
      --count;
    }
  }
}

template <class T>
Pix VHSet<T>::add(const T& item) {
  if (tableTooCrowded(count, size))
    resize();

  unsigned int bestspot = size;
  unsigned int hashval = hash(item);
  unsigned int h = hashval % size;
  for (unsigned int i = 0; i <= size; ++i) {
    if (status[h] == EMPTYCELL) {
      if (bestspot >= size) bestspot = h;
      tab[bestspot] = item;
      status[bestspot] = VALIDCELL;
      ++count;
      return Pix(&tab[bestspot]);
    }
    else if (status[h] == DELETEDCELL) {
      if (bestspot >= size) bestspot = h;
    }
    else if (tab[h] == item)
      return Pix(&tab[h]);

    if (i == 0)
      h = (h + doublehashinc(hashval, size)) % size;
    else if (++h >= size)
      h -= size;
  }
  tab[bestspot] = item;
  status[bestspot] = VALIDCELL;
  ++count;
  return Pix(&tab[bestspot]);
}

template <class T>
Pix VHSet<T>::seek(const T& key) const {
  unsigned int hashval = hash((T)key);
  unsigned int h = hashval % size;
  for (unsigned int i = 0; i <= size; ++i) {
    if (status[h] == EMPTYCELL)
      break;
    else if (status[h] == VALIDCELL && key == tab[h])
      return Pix(&tab[h]);
    if (i == 0)
      h = (h + doublehashinc(hashval, size)) % size;
    else if (++h >= size)
      h -= size;
  }
  return 0;
}

template <class T>
void VHSet<T>::del(const T& key) {
  unsigned int hashval = hash(key);
  unsigned int h = hashval % size;
  for (unsigned int i = 0; i <= size; ++i) {
    if (status[h] == EMPTYCELL)
      return;
    else if (status[h] == VALIDCELL && key == tab[h]) {
      status[h] = DELETEDCELL;
      --count;
      return;
    }
    if (i == 0)
      h = (h + doublehashinc(hashval, size)) % size;
    else if (++h >= size)
      h -= size;
  }
}
