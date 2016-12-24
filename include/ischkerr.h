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

    C-isam functions have no exceptions.  These are simple macros
    that check the return code is C-isam calls and raise a BMS
    "errenv" style C exception for untrapped errors.
*/
#include <errenv.h>
#define chk(i,m) ( ((i)<0)?ischkerr((m),__LINE__,what):0 )
#define ISDUP	1
#define	ISNOKEY	1
#define	ISEOF	1
#define ISLOKD	2
#define ISNOREC 4
extern int ischkerr(int,int,const char *);
#define chkread(fd,buf,mode,err)     chk(isread(fd,(char *)&buf,mode),err)
#define chkwrite(fd,buf,err)         chk(iswrite(fd,(char *)&buf),err)
#define chkrewrite(fd,buf,err)       chk(isrewrite(fd,(char *)&buf),err)
#define chkdelete(fd,buf,err)        chk(isdelete(fd,(char *)&buf),err)
#define chkstart(fd,k,len,buf,m,err) chk(isstart(fd,k,len,(char *)&buf,m),err)
#define chkopen(fd,nam,mode)        \
	do {if ((fd=isopen(nam,mode))==-1)\
	    errvdesc(iserrno,"open %s in %s line %d",nam,what,__LINE__);\
	   } while (0)
#define chkopenx(fd,nam,mode,sz)	\
	do {if ((fd=isopenx(nam,mode,sz))==-1)\
	    errvdesc(iserrno,"open %s in %s line %d",nam,what,__LINE__);\
	   } while (0)
