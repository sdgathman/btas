#LIBS = -lbtas -lbms -lmc -lbms		# for MEDICOMP
LIBS = -lbtas -lbms -lm		# for most others
#CFLAGS = -O -I$(BMS)/include		# if not in environment
#LDFLAGS = -g -L$(BMS)/lib

.DEFAULT:
	co -rV129 $(<:.o=.c)

OBJS = parse.o exec.o vwsel.o vwopt.o join.o version.o cursor.o	\
       istable.o sql.o column.o order.o like.o coldate.o fdbl.o selcol.o \
       system.o func.o btdir.o lit.o values.o sqltype.o

sql:	main.o $(OBJS)
	$(CC) $(LDFLAGS) main.o $(OBJS) $(LIBS) -o sql

sqltype.o : sqltype.c sql.h
values.o : values.c cursor.h
vwsel.o : vwsel.c object.h vwopt.h sql.h obmem.h sql.def cursor.h column.h 
order.o : order.c object.h cursor.h column.h sql.h obmem.h sql.def 
version.o : version.c config.h 
exec.o : exec.c cursor.h column.h sql.h obmem.h sql.def object.h 
vwopt.o : vwopt.c obmem.h vwopt.h sql.h sql.def column.h object.h 
sql.o : sql.c config.h sql.h obmem.h sql.def object.h column.h 
like.o: like.c config.h
join.o : join.c object.h cursor.h column.h sql.h obmem.h sql.def 
btdir.o : btdir.c cursor.h column.h sql.h obmem.h sql.def coldate.h \
	config.h 
parse.o : parse.c sql.h obmem.h sql.def 
func.o : func.c sql.h obmem.h sql.def 
lit.o : lit.c sql.h obmem.h sql.def 
istable.o : istable.c cursor.h column.h sql.h obmem.h sql.def object.h \
	coldate.h cisam.h 
column.o : column.c config.h cursor.h column.h sql.h obmem.h sql.def \
	coldate.h object.h 
coldate.o : coldate.c config.h column.h sql.h obmem.h sql.def coldate.h \
	object.h 
cursor.o : cursor.c config.h object.h cursor.h column.h sql.h obmem.h \
	sql.def coldate.h 

cisam.h:
	@echo You must find and link "cisam.h" into this directory.; exit 1

parse.c:	parse.y
	yacc -vl parse.y
	mv y.tab.c parse.c

tar:
	tar cvf sql.tar *.c *.h *.def makefile *.y
	compress -v sql.tar

Sql.o:	Sql.cc /bms/include/Cursor.h

libsql.a:	Sql.o $(OBJS)
	ar rv $@ $?