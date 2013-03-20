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
#include <btas.h>

char btopflags[32] = {	/* flags required by operation */
	/* read operations */
  BT_READ, BT_READ, BT_READ, BT_READ, BT_READ, BT_READ, BT_READ, BT_READ,
	/* write,rewrite,delete */
  BT_UPDATE, BT_UPDATE, BT_UPDATE, BT_UPDATE, BT_UPDATE, BT_UPDATE,
	/* open, create, seek, link, touch, stat, flush */
  0, BT_UPDATE, BT_READ|BT_UPDATE, BT_UPDATE, BT_OPEN, 0, 0,
	/* mount, join, unjoin, close, umount */
  BT_UPDATE, BT_UPDATE, BT_UPDATE, 0, BT_UPDATE
};

