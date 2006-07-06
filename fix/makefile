F=btfix.a
CXXFLAGS = $(CFLAGS)

L=-lbtas

all:	$F btreload btrcvr btfree btddir btsave btrest btdb

.PRECIOUS:	$F

$F:	$F(getblock.o)	\
	$F(fsio.o)	\
	$F(blktype.o)	\
	$F(delblock.o)	\
	$F(btfind.o)	\
	$F(logdir.o)	\
	$F(rcvr.o)	\
	$F(fs.o)	\
	$F(Obstack.o)	\
	$F(../util/utilio.o)

#	$F(s1fs.o)	\
#	$F(s1tar.o)

btrcvr:	btrcvr.o $F
	g++ $(LDFLAGS) btrcvr.o $F $L -lbms -o btrcvr

btreload: btreload.o  $F
	g++ $(LDFLAGS) btreload.o  $F $L -lbms -o btreload

btfree:	btfree.o $F
	g++ $(LDFLAGS) btfree.o $F -o btfree

#btfree.o: btfree.c fs.h fix.h
#	g++ $(CPPFLAGS) -c btfree.c

btddir: btddir.o $F
	g++ $(LDFLAGS) btddir.o $F $L -lbms -o btddir

btddir1: btddir1.o s1fs.o $F
	g++ -L/bms/lib -g btddir1.o s1fs.o $F -lptrep -lg++ $L -lbms -o btddir1

btddir1.o: fs.h s1fs.h
btddir.o: fs.h 

btsave: btsave.o $F
	g++ $(LDFLAGS) btsave.o $F $L -lbms -o btsave

btrest:	btrest.o $F
	g++ $(LDFLAGS) btrest.o $F $L -lbms -o btrest

btundup btzap btlink: %: %.o
	$(CC) $(LDFLAGS) $^ $L -lbms -o $@

btdb:	btdb.o getblock.o blktype.o btfix.a
	g++ -g -L/bms/lib btdb.o getblock.o blktype.o btfix.a \
	-lb++ -lbtas -lfsp -lbms -lcurses -o btdb

tar:	*.c *.h
	tar cvf fix.tar *.c *.h makefile; compress -v fix.tar

depend::
	gcc -MM $(CFLAGS) *.cc *.c >depend
