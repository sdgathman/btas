L = $Mlibbtas.a

make:	$L

.PRECIOUS:	$L

$L:	$L(flong.o)	\
	$L(fmoney.o)	\
	$L(FpicM.o)	\
	$L(fnum.o)	\
	$L(fchar.o)	\
	$L(client.o)	\
	$L(btflags.o)	\
	$L(btopen.o)	\
	$L(btchdir.o)	\
	$L(btpathopt.o)	\
	$L(pathopt.o)	\
	$L(findfile.o)	\
	$L(btcreate.o)	\
	$L(btkill.o)	\
	$L(btclear.o)	\
	$L(match.o)	\
	$L(btstat.o)	\
	$L(btlstat.o)	\
	$L(pack.o)	\
	$L(ebtconv.o)	\
	$L(block.o)	\
	$L(btsync.o)	\
	$L(bttemp.o)	\
	$L(btlen.o)	\
	$L(packstr.o)	\
	$L(btlock.o)	\
	$L(btwalk.o)	\
	$L(opendir.o)	\
	$L(basename.o)	\
	$L(btmkdir.o)	\
	$L(btrmdir.o)	\
	$L(btlink.o)	\
	$L(btslink.o)	\
	$L(btfsstat.o)	\
	$L(btflds.o)
	ranlib $L

$L(bttemp.o) $L(btflds.o) $L(pack.o):	../include/btflds.h
testlib:	testlib.o
	gcc -g testlib.o -o testlib -lcheck $L -L/bms/lib -lbms
