CXXFLAGS=$(CFLAGS)
#L = $(BMS)/slib/$Mlibbtas.a
L = ../lib/$Mlibbtas.a
LIBS =  -L../lib -L/bms/lib -lbtas -lbms

UTILOBJS=	btutil.o	\
		dump.o		\
		utilio.o 	\
		archive.o 	\
		utar.o		\
		copy.o 		\
		delete.o	\
		unlock.o	\
		change.o	\
		mount.o		\
		help.o		\
		permstr.o	\
		version.o

make:	btutil btar btpwd btdu # isserve idx dat

btutil:	$(UTILOBJS) $L
	$(CC) -map $(UTILOBJS) \
	$(LDFLAGS) -lm $(LIBS) -o btutil

btar:	btar.o archive.o permstr.o
	$(CC) btar.o archive.o permstr.o $(LDFLAGS) $(LIBS) -o btar

btinit:	btinit.o utilio.o
	g++ btinit.o utilio.o $(LDFLAGS) -lbtas -lbms -o btinit

btpwd:  btpwd.o
	$(CC) $(LDFLAGS) btpwd.o $L -lbms -o btpwd

btdu:	btdu.o
	$(CC) $(LDFLAGS) btdu.o -lbtas -lbms -o btdu

btaddfld:	btaddfld.o
	$(CC) $(LDFLAGS) btaddfld.o -lbtas -lbms -o btaddfld

check:	check.o
	$(CC) $(LDFLAGS) check.o $(LIBS) -o check

isserve:	isserve.c $L
	$(CC) $(CFLAGS) isserve.c $(LDFLAGS) $(LIBS) -o isserve

$(UTILOBJS):	btutil.h

dat:	dat.o $L
	$(CC) $(LDFLAGS) dat.o $(LIBS) -o dat

idx:	idx.o fcbread.o $L
	$(CC) $(LDFLAGS) idx.o fcbread.o -lsisam $(LIBS) -o idx

btflded: btflded.o $L
	$(CC) $(LDFLAGS) btflded.o -lfsp -lfs -lbtas -lbms -lcurses -o btflded

tar:
	tarx cvfz btutil.tar.Z *.c makefile *.h *.sh *.help
