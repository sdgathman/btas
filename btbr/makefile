all:	btbr btflded

browsebt.o: browse.c
	$(CC) $(CFLAGS) -DBTASX -c browse.c -o browsebt.o

btbr:	browsebt.o ddread.o fcbread.o obstack.o etoa.o btfread.o version.o
	$(CC) browsebt.o ddread.o fcbread.o obstack.o etoa.o btfread.o \
	      version.o \
	$(LDFLAGS) -lfs -lfsp -lfsp -lbtas -lbms -lcurses -o btbr
#	$(LDFLAGS) -lfs -lfsp -lfsp -lbtas -ljls -lbms -lcurses -o btbr

btflded: btflded.o $L
	$(CC) $(LDFLAGS) btflded.o -lfs -lfsp -lbtas -lbms -lcurses -o btflded

browseis.o: browse.c
	$(CC) $(CFLAGS) -DCISAM -c browse.c
	mv browse.o browseis.o

isbr:	browseis.o fcbread.o obstack.o etoa.o version.o
	$(CC) browseis.o fcbread.o obstack.o etoa.o \
	      version.o \
	$(LDFLAGS) -lfs -lisam -lbms -lcurses -o isbr

isbrs:	browseis.o fcbread.o obstack.o etoa.o
	$(CC) browseis.o fcbread.o obstack.o etoa.o \
	$(LDFLAGS) -lfs -lsisam -lbms -lcurses -o isbrs

fcbtodd:	fcbtodd.o fcbread.o obstack.o
	$(CC) fcbtodd.o fcbread.o obstack.o \
	$(LDFLAGS) -lbtas -lbms -o fcbtodd

btfread:	btfread.o
	$(CC) btfread.o $(LDFLAGS) -lbtas -lbms -o btfread

fcbtodds:	fcbtodd.o fcbread.o obstack.o
	$(CC) fcbtodd.o fcbread.o obstack.o \
	$(LDFLAGS) -lsisam -lbms -o fcbtodds

fcbread.o:	fcbread.c fld.h
