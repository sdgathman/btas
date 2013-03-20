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

#ifndef ANSI
#define const
#endif

/* Implement in assembler ASAP.  Actually, don't bother.  The 
 * assembler versions were important on the 8086, 80x86, and 680x0
 * processors.  However, modern RISC processors do just as well
 * with the C version.  Modern Intel processors are really RISC
 * processors with an Intel ISA emulator in microcode, and using
 * their "string" assembler ops is actually slower than the output 
 * of the GNU C compiler. */

/** Return index of first differing char.  This is the key primitive
 * used by the BTAS algorithm.  There is no equivalent in C libraries. 
 * @param s1 pointer to beginning of first block to compare
 * @param s2 pointer to beginning of second block to compare
 * @len number of bytes to compare
 */
int blkcmp(s1,s2,len)	
  const unsigned char *s1, *s2;
  int len;
{
  register int i = len;
  while (i && *s1++ == *s2++) --i;
  return len - i;
}

/** Move block left.   This can be replaced by memmove(dst-len,src-len,len) in
 * modern C libraries, which handles overlapping ranges, and is optimized to
 * move by words after handling all the fiddly boundary conditions.  
 * @param dst pointer to the first char <emph>after</emph> the last to write.
 * @param src pointer to the first char <emph>after</emph> the last to copy.
 * @param len the number of chars to copy
 */
void blkmovl(dst,src,len)
  char *dst;
  const char *src;
  int len;
{
  while (len--) *--dst = *--src;
}
