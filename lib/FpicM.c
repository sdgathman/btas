/* 
    This file is part of the BTAS client library.

    The BTAS client library is free software: you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of the License,
    or (at your option) any later version.

    BTAS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with BTAS.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <money.h>

/** Direct format a MONEY variable from file format input to output buffer.
  A pointer to a static MONEY variable is returned to allow the user to use
  addM,subM, etc.  
  @see pic()
  @param p	source FMONEY (MONEY in file format)
  @param m	mask to use with pic
  @param b	output character buffer
  @return	pointer to static MONEY
 */ 
MONEY *FpicM(p,m,b)
  const FMONEY p;	/* MONEY variable in FILE format */
  const char *m;	/* mask */
  char *b;	/* buffer postion */
{ static MONEY jlsM;
  jlsM = ldM(p);
  (void)pic(&jlsM,m,b);
  return &jlsM;
}

