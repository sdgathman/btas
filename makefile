VERS = 2.11.1
CVSTAG = btas-2_11_1

OBJS =	btree.o btbuf.o node.o find.o insert.o btas.o hash.o version.o	\
	btfile.o btkey.o assert.o server.o btdev.o fsdev.o alarm.o	\
	btserve.o LockTable.o

L = lib/libbtas.a
CC=g++
CXXFLAGS=$(CFLAGS)
BMSLIB=/bms/lib/libbms.a	# static libbms

make:	btserve btstop btstat btinit $L

btserve: $(OBJS) $L
	g++ $(LDFLAGS) $(OBJS) $L $(BMSLIB) -o btserve

nohash.o hash.o:	btree.h

$L:	lib.done cisam.done

lib.done:
	cd lib; CC=gcc CFLAGS="$(CFLAGS) -I../include" $(MAKE) && touch lib.done

cisam.done:
	cd cisam; CC=gcc CFLAGS="$(CFLAGS) -I../include" $(MAKE) lib && touch cisam.done

btstop:	btstop.c include/btas.h
	$(CC) $(CFLAGS)  $< -o btstop

btinit:	btinit.c btbuf.h
	$(CC) $(CFLAGS) $(LDFLAGS) $< $(BMSLIB) -o btinit

SRCTAR=btas-$(VERS).src.tar.gz
PKG=btas-$(VERS)

$(SRCTAR):	
	cvs export -r$(CVSTAG) -d $(PKG) btas
	tar cvf $(PKG).src.tar $(PKG)
	gzip -f $(PKG).src.tar
	rm -r $(PKG)

tar:	$(SRCTAR)

rpm:	$(SRCTAR)
	mv btas-$(VERS).src.tar.gz /bms/rpm/SOURCES
	rpm -ba btas.spec

depend::
	gcc -MM $(CFLAGS) *.cc *.c >depend
