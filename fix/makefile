F=btfix.a
CXXFLAGS = $(CFLAGS)

L=-lbtas

all:	$F btreload btrcvr btfree btddir btsave btrest

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
	g++ $(LDFLAGS) btrcvr.o $F $L -lg++ -lbms -o btrcvr

btreload: btreload.o pt.o $F
	g++ $(LDFLAGS) btreload.o pt.o $F $L -lg++ -lbms -o btreload

btfreeze: btfreeze.o
	$(CC) $(LDFLAGS) btfreeze.o -lbtas -lbms -o btfreeze

btfree:	btfree.o $F
	g++ $(LDFLAGS) btfree.o $F -o btfree

#btfree.o: btfree.c fs.h fix.h
#	g++ $(CPPFLAGS) -c btfree.c

btddir: btddir.o $F
	g++ $(LDFLAGS) btddir.o $F $L -lg++ -lbms -o btddir

btddir1: btddir1.o s1fs.o $F
	g++ -L/bms/lib -g btddir1.o s1fs.o $F -lptrep -lg++ $L -lbms -o btddir1

btddir1.o: fs.h fix.h s1fs.h
btddir.o: fs.h fix.h

btsave: btsave.o $F
	$(CC) $(LDFLAGS) btsave.o $F $L -lbms -o btsave

btrest:	btrest.o $F
	$(CC) $(LDFLAGS) btrest.o $F $L -lbms -o btrest

btundup btzap btlink: %: %.o
	$(CC) $(LDFLAGS) $^ $L -lbms -o $@

btdb:	btdb.o getblock.o blktype.o
	g++ -g -L/bms/lib btdb.o getblock.o blktype.o	\
	-lb++ -lg++ -lbtas -lfsp -lbms -lcurses -o btdb

tar:	*.c *.h
	tar cvf fix.tar *.c *.h makefile; compress -v fix.tar

depend::
	gcc -MM $(CFLAGS) *.cc *.c >depend
