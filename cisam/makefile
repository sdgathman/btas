#L = $(BMS)/slib/libisam.a
L = ../lib/$Mlibbtas.a
R = 100

all:	bttests bttestc btteste isserve istrace

cisam.so:	cisammodule.c
	python2 setup.py build

test:	bttestc btteste
	bttestc $R | tee log
	btteste $R | tee -a log

testcisam:	testcisam.o $L
	$(CC) testcisam.o $(LDFLAGS) -lbtas -lbms -lcheck -o testcisam

btteste:	bttestc.o $L
	$(CC) bttestc.o $(LDFLAGS) -lbtas -lbms -o btteste

bcheck:	bcheck.o $L
	$(CC) bcheck.o $(LDFLAGS) -lbtas -lbms -o bcheck

addindex:	addindex.o $L
	$(CC) addindex.o $(LDFLAGS) -lbtas -lbms -o addindex

indexinfo:	indexinfo.o $L
	$(CC) indexinfo.o $(LDFLAGS) -lbtas -lbms -o indexinfo

bttestc:	bttestc.o
	$(CC) bttestc.o $(LDFLAGS) -lisam -lbms -o bttestc

bttests:	bttestcs.o
	$(CC) bttestcs.o $(LDFLAGS) libsisam.a -lbms -o bttests

bttestcs.o:	bttestcs.c bttestc.c

isserve:	isserve.o $L
	$(CC) isserve.o $(LDFLAGS) $L -lbtas -lbms -o isserve

istrace:	istrace.o
	$(CC) istrace.o $(LDFLAGS) -lbtas -lbms -o istrace

.PRECIOUS:	$L

lib:	$L

$L:	$L(isopen.o)	\
	$L(isread.o)	\
	$L(iswrite.o)	\
	$L(isstart.o)	\
	$L(isdelete.o)	\
	$L(isrewrit.o)	\
	$L(isconv.o)	\
	$L(isdummy.o)	\
	$L(isbuild.o)	\
	$L(iserase.o)	\
	$L(isrename.o)	\
	$L(isindex.o)	\
	$L(ischkerr.o)	\
	$L(islock.o)	\
	$L(rotate.o)	\
	$L(isrange.o)	\
	$L(isdict.o)	\
	$L(isunique.o)	\
	$L(isreq.o)	\
	$L(isparm.o)
