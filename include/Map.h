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

#ifndef _Map_h
#ifdef __GNUC__
#pragma interface
#endif
#define _Map_h 1

#include <builtin.h>
#include <Pix.h>

template <class T,class C> class Map {
protected:
  int                   count;
  C                     def;

public:
                        Map(const C& dflt): def(dflt) { count = 0; }
  virtual              ~Map() {}

  int		length() const { return count; } // current number of items
  bool           empty() const { return count == 0; }

  virtual bool           contains(const T& key) const;      // is key mapped?

  virtual void          clear();                 // delete all items

  virtual C&          operator [] (const T& key) = 0; // access contents by key

  virtual void          del(const T& key) = 0;       // delete entry

  virtual Pix         first() const = 0;     // Pix of first item or 0
  virtual void        next(Pix& i) const = 0;        // advance to next or 0
  virtual const T&          key(Pix i) const = 0;          // access key at i
  virtual C&          contents(Pix i) = 0;     // access contents at i

  virtual bool owns(Pix idx) const {		// is i a valid Pix  ?
    if (idx == 0) return 0;
    for (Pix i = first(); i; next(i)) if (i == idx) return 1;
    return 0;
  }
  virtual Pix   seek(const T& key) const;          // Pix of key

  C&                  dflt() { return def; }	// access default val

  void                  error(const char* msg) const;
  virtual bool           OK() const = 0;                // rep invariant
};
#endif
