OBJS =	btree.o btbuf.o node.o find.o insert.o btas.o hash.o version.o	\
	btfile.o btkey.o assert.o server.o btdev.o fsdev.o alarm.o

L = /bms/lib/$Mlibbtas.a
CC=g++
CXXFLAGS=$(CFLAGS)

.DEFAULT:
	co -rR2V02 $(<:.o=.c)

make:	btserve btstop btinit $L

btserve: $(OBJS) $L
	gcc $(LDFLAGS) $(OBJS) -lbtas -lbms -o btserve

nohash.o hash.o:	btree.h

$L:
	cd lib; $(MAKE)

btstop:	btstop.c include/btas.h
	$(CC) $(CFLAGS)  $< -o btstop

btinit:	btinit.c btbuf.h
	$(CC) $(CFLAGS) $(LDFLAGS) $< -lbms -o btinit

.c.ln:
	lint -c -I/bms/include $*.c >>lint

.SUFFIXES: .ln

lint:	lintserv linttest lintinit

lintinit:	btinit.ln btbuf.h llibbtas.ln
	lint btinit.ln llibbtas.ln >>lint

btas.ln btopen.ln server.ln:	include/btas.h include/bterr.h btree.h

tar:
	find . ! -name "*.[oaZ]" ! -name "[sp].*" ! -type d ! -perm -1  \
	! -name "*,v" ! -name "*.bt" -print \
	| tarx cvTfz - btas.tar.Z

depend::
	gcc -MM $(CFLAGS) *.cc *.c >depend
