VERS = 2.10.2

OBJS =	btree.o btbuf.o node.o find.o insert.o btas.o hash.o version.o	\
	btfile.o btkey.o assert.o server.o btdev.o fsdev.o alarm.o	\
	btserve.o LockTable.o

L = lib/libbtas.a
CC=g++
CXXFLAGS=$(CFLAGS)
BMSLIB=/bms/lib/libbms.a	# static libbms

make:	btserve btstop btstat btinit $L

btserve: $(OBJS) $L
	gcc $(LDFLAGS) $(OBJS) $L $(BMSLIB) -o btserve

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

$(SRCTAR):	
	rm btas-$(VERS); ln -s . btas-$(VERS)
	for dir in . sql util lib cisam include fix btbr; do \
	   path=btas-$(VERS)/$$dir; \
    ls $$path/*.[chy] $$path/*.cc $$path/makefile $$path/*.spec $$path/*.sh \
	  $$path/*.scr $$path/*.def; done |	\
	  tar cvT - -f - | gzip >btas-$(VERS).src.tar.gz

tar:	$(SRCTAR)

rpm:	$(SRCTAR)
	mv btas-$(VERS).src.tar.gz /bms/rpm/SOURCES
	rpm -ba btas.spec

depend::
	gcc -MM $(CFLAGS) *.cc *.c >depend
