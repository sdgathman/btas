Summary: The BMS BTree Access filesystem (BTAS)
Name: btas
%define version 2.10.1
Version: %{version}
Release: 4
Copyright: Commercial
Group: System Environment/Base
Source: file:/linux/btas-%{version}.src.tar.gz
BuildRoot: /var/tmp/%{name}-root
BuildRequires: libbms-devel >= 1.1.3

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

%build
CFLAGS="$RPM_OPT_FLAGS -I./include -I/bms/include" make
CC=gcc; export CC
cd lib
%ifos aix4.1
ln libbtas.a PIClibbtas.a
%else
M=PIC CFLAGS="$RPM_OPT_FLAGS -I../include -I/bms/include -fpic" make
cd ../cisam
M=PIC CFLAGS="$RPM_OPT_FLAGS -I../include -I/bms/include -fpic" \
	make lib
cd -
%endif
mkdir pic; cd pic; ar xv ../PIClibbtas.a; cd ..
gcc -shared -o libbtas.so pic/*.o -L/bms/lib -lbms
cd ../cisam
LDFLAGS="-s -L../lib -L/bms/lib" CFLAGS="$RPM_OPT_FLAGS -I../include -I/bms/include" make isserve bcheck addindex indexinfo
cd ../util
LDFLAGS="-s -L../lib -L/bms/lib" CFLAGS="$RPM_OPT_FLAGS -I../include -I/bms/include" make
cd ../sql
LDFLAGS="-s -L../lib -L/bms/lib" CFLAGS="$RPM_OPT_FLAGS -I../include -I/bms/include" make
cd ../fix
LDFLAGS="-s -L../lib -L/bms/lib" CFLAGS="$RPM_OPT_FLAGS -I../include -I/bms/include" make
cd ../btbr
LDFLAGS="-s -L../lib -L/bms/lib" CFLAGS="$RPM_OPT_FLAGS -I../include -I/bms/include" make

%install
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
cp include/[a-z]*.h $RPM_BUILD_ROOT/bms/include
mkdir -p $RPM_BUILD_ROOT/bms/fbin
cp util/btcd.sh $RPM_BUILD_ROOT/bms/fbin/btcd
cp sql/btl.sh $RPM_BUILD_ROOT/bms/bin/btl
cp sql/btlc.sh $RPM_BUILD_ROOT/bms/bin/btlc
cp sql/sql $RPM_BUILD_ROOT/bms/bin
cp cisam/isserve cisam/bcheck cisam/addindex cisam/indexinfo	\
	$RPM_BUILD_ROOT/bms/bin
cp btbr/btbr btbr/btflded $RPM_BUILD_ROOT/bms/bin

{ cd $RPM_BUILD_ROOT/bms/bin
  strip btserve btinit btstop || true
  strip btsave btddir btreload btfree btrcvr btrest || true
  strip btutil btar btdu btpwd sql || true
  strip btbr btflded || true
  strip isserve bcheck || true
  ln addindex delindex
}

%clean
rm -rf $RPM_BUILD_ROOT

%pre
%ifos aix4.1
mkuser -a id=711 pgrp=bms home=/bms \
	gecos="BTAS/X File System" btas 2>/dev/null || true
%endif

%files
%defattr(-,btas,bms)
%dir /bms/bin
%config /bms/bin/btstart
/bms/bin/btar
/bms/bin/btddir
/bms/bin/btdu
/bms/bin/btfree
/bms/bin/btinit
/bms/bin/btpwd
/bms/bin/btreload
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
/bms/bin/indexinfo
%attr(2755,btas,bms)/bms/bin/isserve
%dir /bms/lib
/bms/lib/libbtas.a
%dir /bms/fbin
/bms/fbin/btcd

%files devel
%defattr(-,btas,bms)
/bms/slib/libbtas.a
/bms/include/*.h

%changelog
* Wed Oct 17 2001 Stuart Gathman <stuart@bmsi.com>
- fix dyn lib packaging for AIX
- auto add btas user
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
