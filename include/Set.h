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


#ifndef _Set_h
#ifdef __GNUG__
#pragma interface
#endif
#define _Set_h 1

#include <Pix.h>

template <class T> class Set {
protected:

  int                   count;

public:
  virtual              ~Set() { }

  int	length() const { return count; } // current number of items
  int                   empty() const { return count == 0; }

  virtual Pix           add(const T& item) = 0;      // add item; return Pix
  virtual void          del(const T& item) = 0;      // delete item
  virtual int           contains(const T& item) const;     // is item in set?

  virtual void          clear();                 // delete all items

  virtual Pix           first() const = 0;             // Pix of first item or 0
  virtual void          next(Pix& i) const = 0;        // advance to next or 0
  virtual T&          operator () (Pix i) = 0; // access item at i
  const T&    operator () (Pix i) const { return (*(Set *)this)(i); }

  virtual int           owns(Pix i) const;             // is i a valid Pix  ?
  virtual Pix           seek(const T& item) const;         // Pix of item

  void                  operator |= (const Set& b); // add all items in b
  void                  operator -= (const Set& b); // delete items also in b
  void                  operator &= (const Set& b); // delete items not in b

  int                   operator == (const Set& b) const;
  int                   operator != (const Set& b) const;
  int                   operator <= (const Set& b) const; 

  void                  error(const char* msg) const;
  virtual int           OK() const = 0;                // rep invariant
};

#endif
