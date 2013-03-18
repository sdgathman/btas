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

BLOCK *uniquekey(BTCB *);	/* find unique key */
BLOCK *btfirst(BTCB *);		/* find first matching key */
BLOCK *btlast(BTCB *);		/* find last matching key */
BLOCK *verify(BTCB *);		/* verify cached location */
int addrec(BTCB *);		/* add user record */
void delrec(BTCB *, BLOCK *);	/* delete user record */
