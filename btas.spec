Summary: The BMS BTree Access filesystem (BTAS)
Name: btas
Version: 2.11.3
Release: 3.el4
License: Commercial
Group: System Environment/Base
Source: file:/linux/btas-%{version}.src.tar.gz
Patch: btas-el4.patch
BuildRoot: /var/tmp/%{name}-root
BuildRequires: libbms-devel >= 1.1.5, libstdc++-devel
# workaround for libb++ bug in lib/client.c 
#Conflicts: libb++ < 3.0.2

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
%patch -p1 -b .el4

%build
CFLAGS="$RPM_OPT_FLAGS -I./include -I/bms/include" make
export CC=gcc
M=PIC CFLAGS="$RPM_OPT_FLAGS -I../include -I/bms/include -fpic" make -C lib 
M=PIC CFLAGS="$RPM_OPT_FLAGS -I../include -I/bms/include -fpic" make -C cisam lib 
LDFLAGS="-s -L../lib -L/bms/lib"
mkdir lib/pic ||:; cd lib/pic; ar xv ../PIClibbtas.a; cd -
cd lib; gcc -shared -o libbtas.so pic/*.o -L/bms/lib -lbms; cd -
export CFLAGS="$RPM_OPT_FLAGS -I../include -I/bms/include"
LDFLAGS="$LDFLAGS" CFLAGS="$CFLAGS" make -C cisam isserve bcheck addindex indexinfo istrace testcisam
LDFLAGS="$LDFLAGS" CFLAGS="$CFLAGS" make -C lib testlib
LDFLAGS="$LDFLAGS" CFLAGS="$CFLAGS" make -C util
LDFLAGS="$LDFLAGS" CFLAGS="$CFLAGS" make -C sql
LDFLAGS="$LDFLAGS" CFLAGS="$CFLAGS" make -C fix
LDFLAGS="$LDFLAGS" CFLAGS="$CFLAGS" make -C btbr

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc/init.d
cp -p btas.rc $RPM_BUILD_ROOT/etc/init.d/btas
mkdir -p $RPM_BUILD_ROOT/etc/logrotate.d
cp -p btas.logrotate $RPM_BUILD_ROOT/etc/logrotate.d/btas
mkdir -p $RPM_BUILD_ROOT/bms/bin
cp btserve btstop btstat btinit $RPM_BUILD_ROOT/bms/bin
cp btstart.sh $RPM_BUILD_ROOT/bms/bin/btstart
cp fix/btsave fix/btddir fix/btreload fix/btfree fix/btrcvr fix/btrest \
	fix/btdb $RPM_BUILD_ROOT/bms/bin
cp util/btutil util/btpwd util/btar util/btdu util/btfreeze \
	$RPM_BUILD_ROOT/bms/bin
cp util/btinit $RPM_BUILD_ROOT/bms/bin/btinitx
mkdir -p $RPM_BUILD_ROOT/bms/lib
cp lib/libbtas.a $RPM_BUILD_ROOT/bms/lib
cp lib/libbtas.so $RPM_BUILD_ROOT/bms/lib
mkdir -p $RPM_BUILD_ROOT/bms/include
cp include/[a-z]*.h cisam/isreq.h $RPM_BUILD_ROOT/bms/include
mkdir -p $RPM_BUILD_ROOT/bms/fbin
cp util/btcd.sh $RPM_BUILD_ROOT/bms/fbin/btcd
mkdir -p $RPM_BUILD_ROOT/usr/share/btas
cp util/btutil.help $RPM_BUILD_ROOT/usr/share/btas
cp sql/btl.sh $RPM_BUILD_ROOT/bms/bin/btl
cp sql/btlc.sh $RPM_BUILD_ROOT/bms/bin/btlc
cp sql/btlx.sh $RPM_BUILD_ROOT/bms/bin/btlx
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
%ifos linux
/usr/sbin/useradd -u 711 -d /bms -M -c "BTAS/X File System" -g bms btas || true

%post
/sbin/ldconfig
/sbin/chkconfig --add btas
%else
%post
%endif
if test ! -f /var/log/btas.log; then
  touch /var/log/btas.log
  chown btas:bms /var/log/btas.log
  chmod 0644 /var/log/btas.log
fi

%files
%defattr(-,btas,bms)
%dir /bms/bin
%config(noreplace) /bms/bin/btstart
%attr(0755,root,root)/etc/init.d/btas
%attr(0644,root,root)/etc/logrotate.d/btas
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
/bms/bin/btdb
/bms/bin/btinitx
/bms/bin/btfreeze
/bms/bin/btflded
/bms/bin/btflded.scr
%attr(2755,btas,bms)/bms/bin/isserve
%dir /bms/lib
/bms/lib/libbtas.so
%dir /bms/fbin
/bms/fbin/btcd

%files devel
%defattr(-,btas,bms)
/bms/lib/libbtas.a
/bms/include/*.h

%changelog
* Thu Jul 30 2009 Stuart Gathman <stuart@bmsi.com> 2.11.3-2
- libbtas: improve server restart recovery
- btserve: allow linking to root dir again
- drop AIX support in spec
* Thu Jul 30 2009 Stuart Gathman <stuart@bmsi.com> 2.11.3-1
- libbtas: detect and recover server restart
- Cisam: enable auto-repair of dupkey on secondary index.
* Thu Jul 30 2009 Stuart Gathman <stuart@bmsi.com> 2.11.2-1
- Add 6-byte timestamp support to btbr and sql.
* Tue Mar 31 2009 Stuart Gathman <stuart@bmsi.com> 2.11.1-1
- Prevent exception for btclose.  Return error code instead.
- btutil: fix keylength for delete.  ('cr bl' 'cr bl.a' 'de bl' -> 211)
* Thu Jun 21 2007 Stuart Gathman <stuart@bmsi.com> 2.11.0-2
- Fix update of key field.
* Thu Jun 21 2007 Stuart Gathman <stuart@bmsi.com> 2.11-1
- Fix issue363 - 203 on REPLACE with tiny key
- Fix issue353 - rename of mounted dir should fail
- Check for big enough cache at startup.
- Support for btas extents bigger than 2G
- Enhance SQL NULL and fix delimited date output.
* Tue Oct 17 2006 Stuart Gathman <stuart@bmsi.com> 2.10.9-2
- provide btinitx to manage extents
* Fri Jun 30 2006 Stuart Gathman <stuart@bmsi.com> 2.10.9-1
- LARGEFILE support
- btfreeze, btdb
* Fri Mar 03 2006 Stuart Gathman <stuart@bmsi.com> 2.10.8-3
- Move log to /var/log and rotate
- provide sysvinit service script
- impose update ordering via fsync on datablocks vs superblock
- stricter record checks in btddir
* Mon May 16 2005 Stuart Gathman <stuart@bmsi.com> 2.10.8-2
- work with new C++ and STL (use standard map and set, use namespace std)
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
