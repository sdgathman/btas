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


#ifndef _VHMap_h
#define _VHMap_h 1
#pragma interface
#include <Map.h>

template <class T,class C>
class VHMap : public Map<T,C> {
  enum { EMPTYCELL, VALIDCELL, DELETEDCELL };
protected:
  T*             tab;
  C*             cont;
  char*          status;
  unsigned int   size;

public:
                VHMap(const C& dflt,unsigned sz);

                VHMap(VHMap& a);
                ~VHMap();

  C&            operator [] (const T& item);

  void          del(const T& key);

  Pix first() const;

  void          next(Pix& i) const;
  const T&          key(Pix i) const;
  C&          contents(Pix i);

  Pix           seek(const T& key) const;
  bool          contains(const T& key) const;

  void          clear();
  void          resize(unsigned newsize);

  bool           OK() const;
};

template <class T,class C>
inline bool VHMap<T,C>::contains(const T& key) const {
  return seek(key) != 0;
}

template <class T,class C>
inline const T& VHMap<T,C>::key(Pix i) const {
  if (i == 0) error("null Pix");
  return *((T*)i);
}

template <class T,class C>
inline C& VHMap<T,C>::contents(Pix i) {
  if (i == 0) error("null Pix");
  return cont[((unsigned)(i) - (unsigned)(tab)) / sizeof(T)];
}

template <class T> extern unsigned hash(const T &);
#endif
