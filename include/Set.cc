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

#ifdef __GNUG__
#pragma interface
#endif
#include <builtin.h>
#include <Set.h>

template <class T>
Pix Set<T>::seek(const T& item) const {
  Set &self = *(Set *)this;
  Pix i;
  for (i = first(); i != 0 && !(self(i) == item); next(i));
  return i;
}

template <class T>
int Set<T>::owns(Pix idx) const {
  if (idx == 0) return 0;
  for (Pix i = first(); i; next(i)) if (i == idx) return 1;
  return 0;
}

template <class T>
void Set<T>::clear() {
  Pix i = first(); 
  while (i != 0) {
    del((*this)(i));
    i = first();
  }
}

template <class T>
int Set<T>::contains (const T& item) const {
  return seek(item) != 0;
}

template <class T>
int Set<T>::operator <= (const Set& b) const {
  Set &self = *(Set *)this;
  if (count > b.count) return 0;
  if (count == 0) return 1;
  for (Pix i = first(); i; next(i)) if (b.seek(self(i)) == 0) return 0;
  return 1;
}

template <class T>
int Set<T>::operator == (const Set& b) const {
  Set &self = *(Set *)this;
  int n = count;
  if (n != b.count) return 0;
  if (n == 0) return 1;
  Pix i = first();
  Pix j = b.first();
  while (n-- > 0) {
    if ((b.seek(self(i)) == 0) || (seek(b(j)) == 0)) return 0;
    next(i);
    b.next(j);
  }
  return 1;
}

template <class T>
int Set<T>::operator != (const Set& b) const {
  return !(*this == b);
}

template <class T>
void Set<T>::operator |= (const Set& b) {
  if (&b != this)
    for (Pix i = b.first(); i; b.next(i)) add(b(i));
}

template <class T>
void Set<T>::operator -= (const Set& b) {
  if (&b == this)
    clear();
  else
    for (Pix i = b.first(); i; b.next(i)) del(b(i));
}

template <class T>
void Set<T>::operator &= (const Set& b) {
  if (&b != this) {
    Pix i = first();
    Pix n = i;
    while (i != 0) {
      next(n);
      if (b.seek((*this)(i)) == 0) del((*this)(i));
      i = n;
    }
  }
}

template <class T>
void Set<T>::error(const char* msg) const {
  (*lib_error_handler)("Set", msg);
}
