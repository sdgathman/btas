VERS = btas-2.14
TAG = $(VERS)

OBJS =	btree.o btbuf.o node.o find.o insert.o btas.o hash.o version.o	\
	btfile.o btkey.o assert.o server.o btdev.o fsdev.o alarm.o	\
	btserve.o LockTable.o

L = lib/libbtas.a
CC=g++
CXXFLAGS=$(CFLAGS)

make:	btserve btstop btstat btinit 

btserve: $(OBJS) $L
	g++ $(LDFLAGS) $(OBJS) $L -o btserve

nohash.o hash.o:	btree.h

$L:	lib.done cisam.done

lib.done:
	CC=gcc CFLAGS="-I../include $(CFLAGS)" $(MAKE) -C lib && touch lib.done

cisam.done:
	CC=gcc CFLAGS="-I../include $(CFLAGS)" $(MAKE) -C cisam lib && touch cisam.done

btstop:	btstop.c include/btas.h
	$(CC) $(CFLAGS)  $< -o btstop

btinit:	btinit.c btbuf.h
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o btinit

SRCTAR=$(VERS).src.tar.gz

$(SRCTAR):	
	git archive --format=tar --prefix=$(VERS)/ -o $(VERS).src.tar $(TAG)
	gzip $(VERS).src.tar

tar:	$(SRCTAR)

rpm:	$(SRCTAR)
	mv $(VERS).src.tar.gz ~/rpm/SOURCES
	rpmbuild -ba btas.spec

srpm:	$(SRCTAR)
	mv $(VERS).src.tar.gz ~/rpm/SOURCES
	rpmbuild -bs btas.spec

depend::
	gcc -MM $(CFLAGS) *.cc *.c >depend
