/* 1.5	DELETE statement operational, INSERT, UPDATE, subqueries parsed */
/* 1.6  INSERT, UPDATE operational */
/* 1.7  DROP TABLE operational */
/* 1.8	update C-isam emulator indexes */
/* 1.9	map identifiers to lower case by default, upper case cmd option */
/* 1.10 comments: // to EOL ignored
	names following '.' are always IDENTs (not keywords) */
/* 1.11 optimize equivalent columns (exec.c) */
/* 1.11.1	PNUM bug, -q option */
/* 1.11.2	select bug */
/* 1.11.3	Column_sql bug */
/* 1.11.4	system table */
/* 1.12		UPPER(), LOWER(), IF() */
/* 1.12.1	some table optimizations */
/* 1.12.2	typo in Ebcdic_dup (oh for C++) */
/* 1.12.3	unknown function, quote in comment bugs */
/* 1.12.4	dayno function */
/* 1.13		NULL values */
/* 1.13.1	minor porting changes from MEDICOMP */
/* 1.13.2	fix assigning 0 to julian */
/* 1.14		Directory tables */
/* 1.14.1	fix a few bugs with directory tables */
/* 1.14.2	add time format to data dictionary */
/* 1.14.3	a few more Directory fields */
/* 1.15		convert nums to strings, fix IF() */
/* 1.16		add ISNULL(), LENGTH(), PIC() */
/* 1.17		GROUP BY, SUM(), CNT(), AVG(), DATEFMT() */
/* 1.18		HAVING works, isrecnum fixed */
/* 1.19		MIN,MAX,PROD work */
/* 1.20		-f option works (Q&D) */
/* 1.20.1	fix stupid // comment bug */
/* 1.21		fix WHERE with no usable key */
/* 1.22		fix Isam_first with no usable key, implement optim! */
/* 1.29.4	fix expressions involving summary/order by columns */
/* 1.29.5	make subqueries read only in DELETE, UPDATE */
/* 1.29.6	[] works in LIKE */
/* 1.29.7	Q&D TIMESTAMP literal */
/* 1.29.8	attempt to fix vwsel problem */
/* 1.29.9	fix decpnt from mask bug */
/* 1.29.10	recognize small fixed point null values
		null Number prints blanks
		default Number mask prints zero
		add timemask sql function
		implement sql_width for many functions */
/* 1.29.11	print version in interactive mode only
		add Time_dup (using Number_dup) to fix core dump
		tweak Dirfield formatting */
/* 1.29.12	fix X literal */
/* $Log$
 * Revision 1.29  1993/03/10  23:22:14  stuart
 * support outer references
 *
 * Revision 1.28  1993/03/05  02:31:09  stuart
 * CASE and SUBSTRING expressions
 *
 * Revision 1.27  1993/02/24  15:53:42  stuart
 * cleanup for new release
 *
 * Revision 1.26  1993/02/19  23:15:25  stuart
 * Fix more storage bugs
 *
 * Revision 1.25  1993/02/19  20:24:33  stuart
 * fix some storage bugs, some ANSI II syntax for table expressions
 * VALUES is dead until values.c finished.
 *
 * Revision 1.24  1993/02/19  03:40:46  stuart
 * sync versions
 *
 * Revision 1.2  1993/02/19  03:33:47  stuart
 * Initial cut at nested queries
 *
 * Revision 1.1  1993/02/17  00:57:10  stuart
 * Initial revision
 *
*/

#include "config.h"

const char version[] = "\
BMS SQL $Revision$\n\
Copyright 1990,1991,1992,1993 Business Management Systems, Inc.";
