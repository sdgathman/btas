Summary: The BMS BTree Access filesystem (BTAS)
Name: btas
%define version 2.10.8
Version: %{version}
Release: 1
Copyright: Commercial
Group: System Environment/Base
Source: file:/linux/btas-%{version}.src.tar.gz
Patch: btas-el3.patch
BuildRoot: /var/tmp/%{name}-root
BuildRequires: libbms-devel >= 1.1.5, libstdc++-devel
%ifos aix4.1
Provides: libbtas.a
%endif

%description
The BTAS filesystem is a hierarchical filesystem where each file and
directory is a btree indexed file.  Records are stored with leading
duplicate compression, and index records are stored with the minumum
unique key length.  As a result, index record are very small regardless
of how large the key is.  In fact, there is no fixed physical key length used
for file access.  High level database libraries create a logical
key length.

%package devel
Summary: Development files for applications which will manipulate BTAS files.
Group: Development/Libraries
Requires: btas = %{version}

%description devel
Headers and libraries needed to develop BTAS applications.

%prep
%setup -q
%patch -p1 -b .el3

%build
%ifos aix4.1
L="BMSLIB=/bms/slib/libbms.a"
%else
L=""
%endif
CFLAGS="$RPM_OPT_FLAGS -I./include -I/bms/include" make $L
CC=gcc; export CC
%ifos aix4.1
ln lib/libbtas.a lib/PIClibbtas.a ||:
LDFLAGS="-s -L../lib -L/bms/lib -Wl,-blibpath:/bms/lib:/lib"
%else
M=PIC CFLAGS="$RPM_OPT_FLAGS -I../include -I/bms/include -fpic" make -C lib
M=PIC CFLAGS="$RPM_OPT_FLAGS -I../include -I/bms/include -fpic" \
	make -C cisam lib
LDFLAGS="-s -L../lib -L/bms/lib"
%endif
mkdir lib/pic ||:; cd lib/pic; ar xv ../PIClibbtas.a; cd -
cd lib; gcc -shared -o libbtas.so pic/*.o -L/bms/lib -lbms; cd -
CFLAGS="$RPM_OPT_FLAGS -I../include -I/bms/include"
LDFLAGS="$LDFLAGS" CFLAGS="$CFLAGS" make -C cisam isserve bcheck addindex indexinfo istrace
LDFLAGS="$LDFLAGS" CFLAGS="$CFLAGS" make -C util
LDFLAGS="$LDFLAGS" CFLAGS="$CFLAGS" make -C sql
LDFLAGS="$LDFLAGS" CFLAGS="$CFLAGS" make -C fix
LDFLAGS="$LDFLAGS" CFLAGS="$CFLAGS" make -C btbr

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/bms/bin
cp btserve btstop btstat btinit $RPM_BUILD_ROOT/bms/bin
cp btstart.sh $RPM_BUILD_ROOT/bms/bin/btstart
cp fix/btsave fix/btddir fix/btreload fix/btfree fix/btrcvr fix/btrest \
	$RPM_BUILD_ROOT/bms/bin
cp util/btutil util/btpwd util/btar util/btdu $RPM_BUILD_ROOT/bms/bin
mkdir -p $RPM_BUILD_ROOT/bms/lib
%ifos aix4.1
mkdir -p $RPM_BUILD_ROOT/bms/slib
cp lib/libbtas.a $RPM_BUILD_ROOT/bms/slib
cp lib/libbtas.so btas.so
ar rv $RPM_BUILD_ROOT/bms/lib/libbtas.a btas.so
%else
cp lib/libbtas.a $RPM_BUILD_ROOT/bms/lib
cp lib/libbtas.so $RPM_BUILD_ROOT/bms/lib
%endif
mkdir -p $RPM_BUILD_ROOT/bms/include
cp include/[a-z]*.h cisam/isreq.h $RPM_BUILD_ROOT/bms/include
mkdir -p $RPM_BUILD_ROOT/bms/fbin
cp util/btcd.sh $RPM_BUILD_ROOT/bms/fbin/btcd
mkdir -p $RPM_BUILD_ROOT/usr/share/btas
cp util/btutil.help $RPM_BUILD_ROOT/usr/share/btas
cp sql/btl.sh $RPM_BUILD_ROOT/bms/bin/btl
cp sql/btlc.sh $RPM_BUILD_ROOT/bms/bin/btlc
cp sql/sql $RPM_BUILD_ROOT/bms/bin
cp cisam/istrace cisam/isserve cisam/bcheck cisam/addindex cisam/indexinfo \
	$RPM_BUILD_ROOT/bms/bin
cp -p btbr/btbr btbr/btflded btbr/btflded.scr $RPM_BUILD_ROOT/bms/bin

{ cd $RPM_BUILD_ROOT/bms/bin
  strip btserve btinit btstop || true
  strip btsave btddir btreload btfree btrcvr btrest || true
  strip btutil btar btdu btpwd sql || true
  strip btbr btflded || true
  strip istrace isserve bcheck || true
  ln addindex delindex
}

%clean
rm -rf $RPM_BUILD_ROOT

%pre
%ifos aix4.1
mkuser -a id=711 pgrp=bms home=/bms \
	gecos="BTAS/X File System" btas 2>/dev/null || true
%endif
%ifos linux
/usr/sbin/useradd -u 711 -d /bms -M -c "BTAS/X File System" -g bms btas || true

%post
/sbin/ldconfig
%endif

%files
%defattr(-,btas,bms)
%dir /bms/bin
%config /bms/bin/btstart
/usr/share/btas/btutil.help
/bms/bin/btar
/bms/bin/btddir
/bms/bin/btdu
/bms/bin/btfree
/bms/bin/btinit
/bms/bin/btpwd
/bms/bin/btreload
/bms/bin/btrcvr
%attr(6755,btas,bms)/bms/bin/btserve
%attr(6755,btas,bms)/bms/bin/btstop
%attr(6755,btas,bms)/bms/bin/btstat
%attr(2755,btas,bms)/bms/bin/btutil
/bms/bin/btsave
/bms/bin/btrest
%attr(2755,btas,bms)/bms/bin/sql
/bms/bin/btl
/bms/bin/btlc
/bms/bin/bcheck
/bms/bin/addindex
/bms/bin/delindex
/bms/bin/istrace
/bms/bin/indexinfo
/bms/bin/btbr
/bms/bin/btflded
/bms/bin/btflded.scr
%attr(2755,btas,bms)/bms/bin/isserve
%dir /bms/lib
%ifos aix4.1
/bms/lib/libbtas.a
%else
/bms/lib/libbtas.so
%endif
%dir /bms/fbin
/bms/fbin/btcd

%files devel
%defattr(-,btas,bms)
%ifos aix4.1
/bms/slib/libbtas.a
%else
/bms/lib/libbtas.a
%endif
/bms/include/*.h

%changelog
* Tue Feb 08 2005 Stuart Gathman <stuart@bmsi.com> 2.10.8-1
- check ulimit when mounting filesystems
- fix error reporting on startup
- fix btchdir when btasdir is NULL
- fix isserve to set isfdlimit for local connection
- fix sql update to rewrite by primary key
* Sat Sep 13 2003 Stuart Gathman <stuart@bmsi.com> 2.10.7-1
- fix iserase of open files
* Wed Aug 06 2003 Stuart Gathman <stuart@bmsi.com>
- fix sql -f with negative numbers
- add sql -e for alternate quote char
* Tue Jul 29 2003 Stuart Gathman <stuart@bmsi.com>
- auto expand fd array for cisam emulator
- isfdlimit call for cisam emulator
* Mon May 19 2003 Stuart Gathman <stuart@bmsi.com>
- fix ebtconv.c and copy.c
* Fri Apr  4 2003 Stuart Gathman <stuart@bmsi.com>
- fix isaddindex bug
* Tue Mar  4 2003 Stuart Gathman <stuart@bmsi.com>
- finish converting to STL and test
* Tue Nov  5 2002 Stuart Gathman <stuart@bmsi.com>
- convert to STL
* Thu Nov 22 2001 Stuart Gathman <stuart@bmsi.com>
- sql bugfixes
* Wed Oct 17 2001 Stuart Gathman <stuart@bmsi.com>
- fix dyn lib packaging for AIX
- auto add btas user
- include btbr, btflded
* Wed Feb 28 2001 Stuart Gathman <stuart@bmsi.com>
- move to CVS
* Fri Sep  8 2000 Stuart Gathman <stuart@bmsi.com>
- include btstat
* Fri Aug  4 2000 Stuart Gathman <stuart@bmsi.com>
- include addindex, delindex, indexinfo
* Mon Jul 10 2000 Stuart Gathman <stuart@bmsi.com>
- include isserve, bcheck
* Wed Jul  5 2000 Stuart Gathman <stuart@bmsi.com>
- uncomment terminate on SIGINT for sql
* Fri Jun  9 2000 Stuart Gathman <stuart@bmsi.com>
- make ".." cross join (mount) boundary on file open
* Mon May 29 2000 Stuart Gathman <stuart@bmsi.com>
- include cisam emulation
- include sql
* Fri May 12 2000 Stuart Gathman <stuart@bmsi.com>
- include more utilities
* Tue May  9 2000 Stuart Gathman <stuart@bmsi.com>
- first RPM release
