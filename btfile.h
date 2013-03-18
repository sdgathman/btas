/*
	Intermediate level key lookup routines.  See btkey.c for
	a more detailed explanation.

    Copyright (C) 1985-2013 Business Management Systems, Inc

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma interface

#include "btree.h"

class BLOCK;
class BlockCache;
struct BTCB;

class bttrace {
  btlevel stack[MAXLEV];
  int getkey(BLOCK *, BLOCK *, char *, t_block *);
  int insert(BLOCK *, BLOCK *, int, char *, int, btlevel *);
protected:
  void btfree(BLOCK *blk);
  BLOCK *btnew(short flags);
  BlockCache *bufpool;
  BLOCK *btbuf(t_block node);
  btlevel *sp;	// FIXME: provide stack interface
public:
  bttrace(BlockCache *);
  BLOCK *trace(struct BTCB *, int, int);
  int needed() const { return sp - stack + 2; }	// worst case blocks needed
  void add(char *urec, int ulen);
  void del();
};

struct btfile: private bttrace {
  btfile(BlockCache *c);
  BLOCK *uniquekey(BTCB *);	/* find unique key */
  BLOCK *btfirst(BTCB *);		/* find first matching key */
  BLOCK *btlast(BTCB *);		/* find last matching key */
  BLOCK *verify(BTCB *);		/* verify cached location */
  bttrace::trace;
  int addrec(BTCB *);		/* add user record */
  void delrec(BTCB *, BLOCK *);	/* delete user record */
  void replace(BTCB *b,bool rew = false);
  //	File and directory handling
  int openfile(BTCB *,int);	/* open/stat file, check security */
  void closefile(BTCB *);		/* close file */
  t_block creatfile(BTCB *);	/* create new file */
  t_block linkfile(BTCB *);	/* link existing file */
  int delfile(BTCB *, t_block);	/* unlink file */
  int join(BTCB *);		/* join file */
  int unjoin(BTCB *);		/* unjoin file */
private:
  int joincnt;
  struct joinrec;
  joinrec *jointbl;
};
