// This may look like C code, but it is really -*- C++ -*-
/* 
Copyright (C) 1988 Free Software Foundation
    written by Doug Lea (dl@rocky.oswego.edu)

This file is part of the GNU C++ Library.  This library is free
software; you can redistribute it and/or modify it under the terms of
the GNU Library General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your
option) any later version.  This library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU Library General Public License for more details.
You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#ifndef _VHSet_h
#ifdef __GNUG__
#pragma interface
#endif
#define _VHSet_h 1

// tableTooCrowded(COUNT, SIZE) is true iff a hash table with COUNT
// elements and SIZE slots is too full, and should be resized.
// This is so if available space is less than 1/8.

#include <Set.h>

template <class T>
class VHSet : public Set<T> {
  /* codes for status fields */
  enum { EMPTYCELL, VALIDCELL, DELETEDCELL };
  static int tableTooCrowded(int cnt, unsigned SIZE) {
    return SIZE - (SIZE >> 3) <= cnt;
  }

protected:
  T*		tab;
  char*         status;
  unsigned int  size;

public:
                VHSet(unsigned int sz = 100);
                VHSet(const Set<T>& a);
  ~VHSet() {
    delete [] tab;
    delete [] status;
  }

  Pix           add(const T& item);

  void          del(const T& key);

  int           contains(const T& item) const { return seek(item) != 0; }

  void          clear() {
    for (unsigned int i = 0; i < size; ++i) status[i] = EMPTYCELL;
    count = 0;
  }

  Pix           first() const {
    for (unsigned int pos = 0; pos < size; ++pos)
      if (status[pos] == VALIDCELL) return Pix(&tab[pos]);
    return 0;
  }

  void          next(Pix& i) const {
    if (i == 0) return;
    unsigned int pos = ((unsigned)i - (unsigned)tab) / sizeof(T) + 1;
    for (; pos < size; ++pos)
      if (status[pos] == VALIDCELL) {
	i = Pix(&tab[pos]);
	return;
      }
    i = 0;
  }
    
  T&          operator () (Pix i) {
    if (i == 0) error("null Pix");
    return *(T *)i;
  }

  Pix           seek(const T& key) const;

  void          operator |= (const VHSet& b);
  void          operator -= (const VHSet& b);
  void          operator &= (const VHSet& b);

  int           operator == (const VHSet& b) const;
  int           operator != (const VHSet& b) const { return ! ((*this) == b); }
  int           operator <= (const VHSet& b) const; 

  int           capacity() const { return size; }
  void          resize(unsigned int newsize = 0);

  int           OK() const {
    int v = tab != 0;
    v &= status != 0;
    int n = 0;
    for (unsigned int i = 0; i < size; ++i) {
      if (status[i] == VALIDCELL) ++n;
      else if (status[i] != DELETEDCELL && status[i] != EMPTYCELL)
	v = 0;
    }
    v &= n == count;
    if (!v) error("invariant failure");
    return v;
  }

};

#endif
