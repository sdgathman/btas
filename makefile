OBJS = btree.o btbuf.o node.o find.o insert.o btas.o hash.o version.o
#OBJS = btree.o btbuf.o node.o find.o insert.o btas.o nohash.o
L = /bms/lib/$Mlibbtas.a

.DEFAULT:
	co -rR1V14 $(<:.o=.c)

make:	btserve btstop btinit $L

btserve:	server.o btfile.o btkey.o assert.o $(OBJS) $L
	$(CC) $(LDFLAGS) server.o btfile.o btkey.o assert.o	\
	$(OBJS) -lbtas -lbms -o btserve

nohash.o hash.o:	btree.h

$L:
	cd lib; $(MAKE)

btstop:	btstop.c include/btas.h
	$(CC) $(CFLAGS) btstop.c -o btstop

btinit:	btinit.c btbuf.h
	$(CC) $(CFLAGS) $(LDFLAGS) btinit.c -lbms -o btinit

.c.ln:
	lint -c -I/bms/include $*.c >>lint

.SUFFIXES: .ln

lint:	lintserv linttest lintinit

lintserv:	btfile.ln btkey.ln server.ln $(OBJS:.o=.ln)
	lint -x btfile.ln btkey.ln server.ln $(OBJS:.o=.ln) llibbtas.ln >>lint

lintinit:	btinit.ln btbuf.h llibbtas.ln
	lint btinit.ln llibbtas.ln >>lint

btas.ln btopen.ln server.ln:	include/btas.h include/bterr.h btree.h

tar:
	find . ! -name "*.[oaZ]" ! -name "[sp].*" ! -type d ! -perm -1  \
	! -name "*,v" ! -name "*.bt" -print \
	| tarx cvTfz - btas.tar.Z
