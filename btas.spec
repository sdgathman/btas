%if 0%{?epel} == 7
%global python3 python36
%else
%global python3 python3
%endif

%if 0%{?fedora} >= 32
%bcond_with python2
%global python2 python27
%else
%if 0%{?epel} < 8
%bcond_without python2
%else
%bcond_with python2
%endif
%global python2 python2
%endif

%if 0%{?rhel} >= 5 && 0%{?rhel} < 7
%global use_systemd 0
%else
%global use_systemd 1
%endif

Summary: The BMS BTree Access filesystem (BTAS)
Name: btas
Version: 2.14
Release: 1%{?dist}
License: Commercial
Group: System Environment/Base
Source: file:/linux/btas-%{version}.src.tar.gz
#Patch: btas-btcd.patch
BuildRoot: /var/tmp/%{name}-root
BuildRequires: libstdc++-devel, gcc-c++, check-devel
BuildRequires: bison, ncurses-devel
# needed to build until libbms references purged
BuildRequires: libbms-devel >= 1.1.7
%if %{use_systemd}
# systemd macros are not defined unless systemd is present
BuildRequires: systemd
%{?systemd_requires}
%else
Requires: chkconfig, daemonize
%endif
Requires: logrotate
# needed to build btdb
#BuildRequires: libb++-devel
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

%if %{with python2}
%package -n %{python2}-btas
Summary: Python module for cisam API
Group: Development/Libraries
Requires: btas = %{version}-%{release}
%{?python_provide:%python_provide %{python2}-btas}
BuildRequires: %{python2}-devel

%description -n python2-btas
Python interface to BTAS Cisam API
%endif

%package -n %{python3}-btas
Summary: Python module for cisam API
Requires: btas = %{version}-%{release}
%{?python_provide:%python_provide %{python3}-btas}
BuildRequires: %{python3}-devel

%description -n %{python3}-btas
Python interface to BTAS Cisam API

%prep
%setup -q
#patch -p1 -b .btcd

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
LDFLAGS="$LDFLAGS" CFLAGS="$CFLAGS" make -e -C sql
LDFLAGS="$LDFLAGS" CFLAGS="$CFLAGS" make -C fix
LDFLAGS="$LDFLAGS" CFLAGS="$CFLAGS" make -C btbr
cd pybtas
%if %{with python2}
%py2_build
%endif
%py3_build
cd -

%check
sh test.sh

%install
rm -rf %{buildroot}
cd pybtas
%if %{with python2}
%py2_install
%endif
%py3_install
cd -
%if %{use_systemd}
mkdir -p %{buildroot}%{_unitdir}
cp -p *.service %{buildroot}%{_unitdir}
%else
mkdir -p %{buildroot}/etc/init.d
cp -p btas.rc %{buildroot}/etc/init.d/btas
%endif
mkdir -p %{buildroot}/etc/logrotate.d
cp -p btas.logrotate %{buildroot}/etc/logrotate.d/btas
mkdir -p %{buildroot}/etc/btas
cp -p btfstab %{buildroot}/etc/btas
mkdir -p %{buildroot}/etc/btas/clrlock.d
BIN="%{_libexecdir}/btas"
mkdir -p %{buildroot}/$BIN
cp btserve btstop btstat btinit %{buildroot}/$BIN
chmod a+x *.sh
cp -p btstart.sh %{buildroot}/$BIN/btstart
cp -p btbackup.sh %{buildroot}/$BIN/btbackup
cp fix/btsave fix/btddir fix/btreload fix/btfree fix/btrcvr fix/btrest \
	%{buildroot}/$BIN
cp util/btutil util/btpwd util/btar util/btdu util/btfreeze \
	%{buildroot}/$BIN
cp util/btinit %{buildroot}/$BIN/btinitx
mkdir -p %{buildroot}%{_libdir}
cp lib/libbtas.a %{buildroot}%{_libdir}
cp lib/libbtas.so %{buildroot}%{_libdir}
mkdir -p %{buildroot}%{_includedir}/btas
cp include/[a-z]*.h cisam/isreq.h %{buildroot}%{_includedir}/btas
mkdir -p %{buildroot}%{_sysconfdir}/profile.d
cp util/btcd.sh %{buildroot}%{_sysconfdir}/profile.d
mkdir -p %{buildroot}/usr/share/btas
cp util/btutil.help %{buildroot}/usr/share/btas
cp sql/btl.sh %{buildroot}/$BIN/btl
cp sql/btlc.sh %{buildroot}/$BIN/btlc
cp sql/btlx.sh %{buildroot}/$BIN/btlx
cp sql/sql %{buildroot}/$BIN
cp cisam/istrace cisam/isserve cisam/bcheck cisam/addindex cisam/indexinfo \
	%{buildroot}/$BIN
cp -p btbr/btbr btbr/btflded btbr/btflded.scr %{buildroot}/$BIN

{ cd %{buildroot}/$BIN
  ln addindex delindex
}
mkdir -p %{buildroot}/var/log/btas
chmod 0775 %{buildroot}/var/log/btas
chmod g+s %{buildroot}/var/log/btas
mkdir -p %{buildroot}%{_sharedstatedir}/btas

mkdir -p %{buildroot}%{_bindir}
ln -sf %{_libexecdir}/btas/btpwd %{buildroot}%{_bindir}
ln -sf %{_libexecdir}/btas/btutil %{buildroot}%{_bindir}
ln -sf %{_libexecdir}/btas/btlc %{buildroot}%{_bindir}
ln -sf %{_libexecdir}/btas/btl %{buildroot}%{_bindir}
ln -sf %{_libexecdir}/btas/sql %{buildroot}%{_bindir}/btsql

%pre
getent group bms > /dev/null || /usr/sbin/groupadd -r -g 101 bms 
getent passwd btas > /dev/null || /usr/sbin/useradd -g bms \
        -c "BTAS/X Database filesystem" \
        -r -d %{_sharedstatedir}/btas -s /sbin/nologin btas
exit 0

%if %{use_systemd}

%post
/sbin/ldconfig
%systemd_post acme-tiny.service acme-tiny.timer

%postun
%systemd_postun_with_restart acme-tiny.service acme-tiny.timer

%preun
%systemd_preun acme-tiny.service acme-tiny.timer

%else

%post
/sbin/ldconfig
/sbin/chkconfig --add btas

%preun
if [ $1 = 0 ]; then
  /sbin/chkconfig --del btas
fi

%endif

%files
%license COPYING
%defattr(-,btas,bms)
%dir /var/log/btas
%config(noreplace) /etc/btas/btfstab
%dir /etc/btas/clrlock.d
%attr(0755,btas,bms) %{_sharedstatedir}/btas
%if %{use_systemd}
%{_unitdir}/*
%else
%attr(0755,root,root)/etc/init.d/btas
%endif
%attr(0644,root,root)/etc/logrotate.d/btas
/usr/share/btas/btutil.help
%{_libexecdir}/btas
%attr(6755,btas,bms)%{_libexecdir}/btas/btserve
%attr(6755,btas,bms)%{_libexecdir}/btas/btstop
%attr(6755,btas,bms)%{_libexecdir}/btas/btstat
%attr(2755,btas,bms)%{_libexecdir}/btas/btutil
%attr(2755,btas,bms)%{_libexecdir}/btas/sql
%attr(2755,btas,bms)%{_libexecdir}/btas/isserve
%attr(2755,btas,bms)%{_libexecdir}/btas/btpwd
%attr(2755,btas,bms)%{_libexecdir}/btas/btbr
%{_bindir}/*
#/bms/bin/btstart
#/bms/bin/btbackup
#/bms/bin/btar
#/bms/bin/btddir
#/bms/bin/btdu
#/bms/bin/btfree
#/bms/bin/btinit
#/bms/bin/btpwd
#/bms/bin/btreload
#/bms/bin/btrcvr
#/bms/bin/btsave
#/bms/bin/btrest
#/bms/bin/btlx
#/bms/bin/bcheck
#/bms/bin/addindex
#/bms/bin/delindex
#/bms/bin/istrace
#/bms/bin/indexinfo
#/bms/bin/btbr
#/bms/bin/btinitx
#/bms/bin/btfreeze
#/bms/bin/btflded
#/bms/bin/btflded.scr
%{_libdir}/libbtas.so
%{_sysconfdir}/profile.d/btcd.sh

%files devel
%defattr(-,btas,bms)
%{_libdir}/libbtas.a
%{_includedir}/btas/*.h

%if %{with python2}
%files -n %{python2}-btas
%license COPYING
%doc README ChangeLog
%{python2_sitearch}/*
%endif

%files -n %{python3}-btas
%license COPYING
%doc README ChangeLog
%{python3_sitearch}/*

%changelog
* Fri Feb  9 2024 Stuart Gathman <stuart@gathman.org> 2.14-1
- Add python subpackages

* Sat Sep  1 2018 Stuart Gathman <stuart@gathman.org> 2.13-3
- put btcd in /etc/profile.d
- add sgid to btpwd
- Add some symlinks in /usr/bin

* Mon Jan  8 2018 Stuart Gathman <stuart@gathman.org> 2.13-2
- Move executables to %%{_libexecdir}/btas
- Add systemd service

* Tue Jan  2 2018 Stuart Gathman <stuart@gathman.org> 2.13-1
- Move libraries to %%{_libdir}
- Move headers to %%{_includedir}/btas

* Wed May 17 2017 Stuart Gathman <stuart@gathman.org> 2.12-3
- Fix 64bit problems in sql
- Add /var/lib/btas and use that as base for filesystems.

* Fri Apr  7 2017 Stuart Gathman <stuart@gathman.org> 2.12-2
- Remove port.h from public interfaces

* Wed Mar 29 2017 Stuart Gathman <stuart@gathman.org> 2.12-1
- 64bit clean, builds and runs on CentOS-7

* Sat Mar  5 2016 Stuart Gathman <stuart@gathman.org> 2.11.6-2
- Fix logrotate
- Add BR: ncurses-devel

* Thu Jul 12 2012 Stuart Gathman <stuart@bmsi.com> 2.11.6-1
- LFS support for btsave,btrest,btddir.
- support nflds and nkflds in sql directory tables
- fix CSV output bugs in sql, issue817

* Wed Mar 14 2012 Stuart Gathman <stuart@bmsi.com> 2.11.5-3
- Fix undefined call to memcpy in insert.c
- Include /bms/etc/clrlock.d

* Fri Mar 09 2012 Stuart Gathman <stuart@bmsi.com> 2.11.5-2
- Compile on EL6

* Thu Apr 21 2011 Stuart Gathman <stuart@bmsi.com> 2.11.5-1
- support CREATE TABLE PRIMARY KEY and CREATE INDEX in sql
- Make btstart and backup use /bms/etc/btfstab

* Wed Jan 19 2011 Stuart Gathman <stuart@bmsi.com> 2.11.4-1
- Cisam: support isbtasinfo
- Cisam: skip bad index records on read (see testBadIndex in DatasetTest.java)

* Mon Jun 28 2010 Stuart Gathman <stuart@bmsi.com> 2.11.3-3
- prevent iserase from erasing directories

* Tue Dec 08 2009 Stuart Gathman <stuart@bmsi.com> 2.11.3-2
- libbtas: improve server restart recovery
- btserve: allow linking to root dir again
- drop AIX support in spec

* Tue Sep 29 2009 Stuart Gathman <stuart@bmsi.com> 2.11.3-1
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
