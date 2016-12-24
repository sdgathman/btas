#ifndef _BTERR_H
#define _BTERR_H
/*	BTAS/X error codes
 *
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

enum {
  BTERROOT =	201,	/* invalid root node */
  BTERFULL =	202,	/* no space for new node */
  BTERINT =	203,	/* internal error (assertion failure) */
  BTERSTK =	204,	/* level stack overflow */
  BTERFREE =	205,	/* free space error */
  BTEROPEN =	206,	/* BTCB already opened */
  BTEROP =	207,	/* invalid operation code */
  BTERNDIR =	208,	/* not a directory */
  BTERBBLK =	209,	/* invalid block id */
  BTERDIR =	210,	/* bad directory operation */
  BTERDUP =	211,	/* duplicate key */
  BTERKEY =	212,	/* key not found */
  BTEREOF =	213,	/* end of file - really BTERKEY */
  BTERLOCK =	214,	/* file locked on open */
  BTERMTBL =	215,	/* no room in mount table */
  BTERMOD =	216,	/* write to readonly file */
  BTERMID =	217,	/* invalid mount id */
  BTERDEL =	218,	/* delete open file or non-empty dir */
  BTERKLEN =	219,	/* invalid key/record length */
  BTERFCNT =	220,	/* bad free count (no room for node) */
  BTERLINK =	221,	/* too many links / symbolic link */
  BTERPERM =	222,	/* security violation */
  BTERBUSY =	223,	/* device busy on BTUMOUNT, BTMOUNT or flush limit */
  BTERJOIN =	224,	/* join table full */
  BTERMBLK =	283,	/* block size too big on mount or invalid super block */
  BTERBMNT =	294,	/* filesystem needs cleaning */
  BTERSERV =	298	/* server not available */
};
#endif
