/***************************************************************************
 *
 *                    RELATIONAL DATABASE SYSTEMS, INC.
 *
 *                            PROPRIETARY DATA
 *
 *  THIS WORK CONTAINS PROPRIETARY INFORMATION AND TRADE SECRETS WHICH ARE 
 *  THE PROPERTY OF RELATIONAL DATABASE SYSTEMS, INC. (RDS).  THIS DOCUMENT 
 *  IS SUBMITTED TO RECIPIENT IN CONFIDENCE.  THE INFORMATION CONTAINED 
 *  HEREIN MAY NOT BE USED, COPIED OR DISCLOSED IN WHOLE OR IN PART EXCEPT 
 *  AS PERMITTED BY WRITTEN AGREEMENT SIGNED BY AN OFFICER OF RDS.
 *
 *  COPYRIGHT (c) 1981, 1986 RELATIONAL DATABASE SYSTEMS, INC., MENLO PARK, 
 *  CALIFORNIA.  All rights reserved.  No part of this work covered by the 
 *  copyright hereon may be reproduced or used in any form or by any means 
 *  -- graphic, electronic, or mechanical, including photocopying, 
 *     recording, taping, or information storage and retrieval systems --
 *  without permission of RDS.
 *
 *  Title:	isam.h
 *  Sccsid:	@(#)isam.h	4.10	5/5/86  23:40:11
 *  Description:
 *		Header file for programs using C-ISAM.
 *
 ***************************************************************************
 */

/*
 *       C-ISAM version 3.00
 *  Indexed Sequential Access Method
 *  Relational Database Systems, Inc.
 */

#ifndef ISAM_INCL		/* avoid multiple include problems */
#define ISAM_INCL
#define CHARTYPE	0
#define DECIMALTYPE	0
#define CHARSIZE	1

#define INTTYPE		1
#define INTSIZE		2

#define LONGTYPE	2
#define LONGSIZE	4

#define DOUBLETYPE	3
#define DOUBLESIZE	(sizeof(double))

#define FLOATTYPE	4
#define FLOATSIZE	(sizeof(float))

#define USERCOLL(x)	((x))

#define COLLATE1	0x10
#define COLLATE2	0x20
#define COLLATE3	0x30
#define COLLATE4	0x40
#define COLLATE5	0x50
#define COLLATE6	0x60
#define COLLATE7	0x70

#define MAXTYPE		5
#define ISDESC		0x80	/* add to make descending type	*/
#define TYPEMASK	0x7F	/* type mask			*/

#ifdef BBN			/* BBN Machine has 10 bits/byte	*/
#define BYTEMASK  0x3FF		/* mask for one byte		*/
#define BYTESHFT  10		/* shift for one byte		*/
#else
#define BYTEMASK  0xFF		/* mask for one byte		*/
#define BYTESHFT  8		/* shift for one byte		*/
#endif

#ifndef	ldint
#define ldint(p)	((short)(((p)[0]<<BYTESHFT)+((p)[1]&BYTEMASK)))
#define stint(i,p)	((p)[0]=(i)>>BYTESHFT,(p)[1]=(i))
#endif

#ifndef	ldlong
long ldlong();
#endif

#ifndef NOFLOAT
#ifndef	ldfloat
/* double	ldfloat(); */
#endif
#ifndef	lddbl
double	lddbl();
#endif
double ldfltnull();
double lddblnull();
#endif

#define ISFIRST		0	/* position to first record	*/
#define ISLAST		1	/* position to last record	*/
#define ISNEXT		2	/* position to next record	*/
#define ISPREV		3	/* position to previous record	*/
#define ISCURR		4	/* position to current record	*/
#define ISEQUAL		5	/* position to equal value	*/
#define ISGREAT		6	/* position to greater value	*/
#define ISGTEQ		7	/* position to >= value		*/
#define ISLESS		8	/* position to < value		*/
#define ISLTEQ		9	/* position to >= value 	*/

/* isread lock modes */
#define ISLOCK     	0x100	/* record lock			*/
#define ISWAIT		0x400	/* wait for record lock		*/
#define ISLCKW		0x500   /* ISLOCK + ISWAIT              */

/* isopen, isbuild lock modes */
#define ISAUTOLOCK	0x200	/* automatic record lock	*/
#define ISMANULOCK	0x400	/* manual record lock		*/
#define ISEXCLLOCK	0x800	/* exclusive isam file lock	*/

#define ISINPUT		0	/* open for input only		*/
#define ISOUTPUT	1	/* open for output only		*/
#define ISINOUT		2	/* open for input and output	*/
#define ISTRANS		4	/* open for transaction proc	*/

/* audit trail mode parameters */
#define AUDSETNAME	0	/* set new audit trail name	*/
#define AUDGETNAME	1	/* get audit trail name		*/
#define AUDSTART	2	/* start audit trail 		*/
#define AUDSTOP		3	/* stop audit trail 		*/
#define AUDINFO		4	/* audit trail running ?	*/

#define MAXKEYSIZE	120	/* max number of bytes in key	*/
#define NPARTS		8	/* max number of key parts	*/

struct keypart
    {
    short kp_start;		/* starting byte of key part	*/
    short kp_leng;		/* length in bytes		*/
    short kp_type;		/* type of key part		*/
    };

struct keydesc
    {
    short k_flags;		/* flags			*/
    short k_nparts;		/* number of parts in key	*/
    struct keypart
	k_part[NPARTS];		/* each key part		*/
		    /* the following is for internal use only	*/
    short k_len;		/* length of whole key		*/
    long k_rootnode;		/* pointer to rootnode		*/
    };
#define k_start   k_part[0].kp_start
#define k_leng    k_part[0].kp_leng
#define k_type    k_part[0].kp_type

#define ISNODUPS  000		/* no duplicates allowed	*/
#define ISDUPS	  001		/* duplicates allowed		*/
#define DCOMPRESS 002		/* duplicate compression	*/
#define LCOMPRESS 004		/* leading compression		*/
#define TCOMPRESS 010		/* trailing compression		*/
#define COMPRESS  016		/* all compression		*/
#define ISCLUSTER 020		/* index is a cluster one       */

struct dictinfo
    {
    short di_nkeys;		/* number of keys defined	*/
    short di_recsize;		/* data record size		*/
    short di_idxsize;		/* index record size		*/
    long di_nrecords;		/* number of records in file	*/
    };

#define EDUPL	  100		/* duplicate record	*/
#define ENOTOPEN  101		/* file not open	*/
#define EBADARG   102		/* illegal argument	*/
#define EBADKEY   103		/* illegal key desc	*/
#define ETOOMANY  104		/* too many files open	*/
#define EBADFILE  105		/* bad isam file format	*/
#define ENOTEXCL  106		/* non-exclusive access	*/
#define ELOCKED   107		/* record locked	*/
#define EKEXISTS  108		/* key already exists	*/
#define EPRIMKEY  109		/* is primary key	*/
#define EENDFILE  110		/* end/begin of file	*/
#define ENOREC    111		/* no record found	*/
#define ENOCURR   112		/* no current record	*/
#define EFLOCKED  113		/* file locked		*/
#define EFNAME    114		/* file name too long	*/
#define ENOLOK    115		/* can't create lock file */
#define EBADMEM   116		/* can't alloc memory	*/
#define EBADCOLL  117		/* bad custom collating	*/
#define ELOGREAD  118		/* cannot read log rec  */
#define EBADLOG   119		/* bad log record	*/
#define ELOGOPEN  120		/* cannot open log file	*/
#define ELOGWRIT  121		/* cannot write log rec */
#define ENOTRANS  122		/* no transaction	*/
#define ENOSHMEM  123		/* no shared memory	*/
#define ENOBEGIN  124		/* no begin work yet	*/
#define ENONFS    125		/* can't use nfs */

/*
 * For system call errors
 *   iserrno = errno (system error code 1-99)
 *   iserrio = IO_call + IO_file
 *		IO_call  = what system call
 *		IO_file  = which file caused error
 */

#define IO_OPEN	  0x10		/* open()	*/
#define IO_CREA	  0x20		/* creat()	*/
#define IO_SEEK	  0x30		/* lseek()	*/
#define IO_READ	  0x40		/* read()	*/
#define IO_WRIT	  0x50		/* write()	*/
#define IO_LOCK	  0x60		/* locking()	*/
#define IO_IOCTL  0x70		/* ioctl()	*/

#define IO_IDX	  0x01		/* index file	*/
#define IO_DAT	  0x02		/* data file	*/
#define IO_AUD	  0x03		/* audit file	*/
#define IO_LOK	  0x04		/* lock file	*/
#define IO_SEM	  0x05		/* semaphore file */

extern int iserrno;		/* isam error return code	*/
extern int iserrio;		/* system call error code	*/
extern long isrecnum;		/* record number of last call	*/
extern char isstat1;		/* cobol status characters	*/
extern char isstat2;
extern char *isversnumber;	/* C-ISAM version number	*/
extern char *iscopyright;	/* RDS copyright		*/
extern char *isserial;		/* C-ISAM software serial number */
extern int  issingleuser;	/* set for single user access	*/
extern int  is_nerr;		/* highest C-ISAM error code	*/
extern char *is_errlist[];	/* C-ISAM error messages	*/
/*  error message usage:
 *	if (iserrno >= 100 && iserrno < is_nerr)
 *	    printf("ISAM error %d: %s\n", iserrno, is_errlist[iserrno-100]);
 */

struct audhead
    {
    char au_type[2];		/* audit record type aa,dd,rr,ww*/
    char au_time[4];		/* audit date-time		*/
    char au_procid[2];		/* process id number		*/
    char au_userid[2];		/* user id number		*/
    char au_recnum[4];		/* record number		*/
    };
#define AUDHEADSIZE  14		/* num of bytes in audit header	*/


#endif
