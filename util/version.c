/* Version Comments:
 1.18  egb, Added EBCDIC translation "e switch" to dump.c
 1.19  sdg, Added file based help
 1.20  sdg, make archive correctly restore owners and permissions
 1.21  egb, make patch usable, minor cleanup in copy
 1.22  sdg, check for zero fbufs in stat, report transaction count
 1.23  sdg, sync now keeps trying when it gets a BUSY error
 1.24  sdg, make switches case independent (again)
 1.25  egb, fix patch so you can add records
 1.26  egb, envelope added around code that opens current directory
 1.27	 sdg, add ebcdic conversion switch to copy/move "co/e"
      make cancel work for copy/move
	      add directory copy (no data records) "co/d"
 1.28  egb, add fatal error message to delete command
 1.29	 sdg, recursive archive
 1.30	 egb, dump fixed to handle outrageous BT_NUM types
 1.31	 sdg, fix some archive bugs
 1.32	 egb, add print working directory (pwd) command
 1.33	 sdg, improve error reporting in archive/load
 1.34	 sdg, replace records option on copy
	      write existing files on wildcard copy
	      allow quit out of help
 1.34.1	 SDG use btopendir in copy.c, hope it still works
 1.35	 SDG manually specify sequential archive output, fix btwalk
	      when starting from the root directory.
 1.36	 SDG setvbuf is broken on m88k :-(
	     restoring permissions now works
 1.37	 SDG LOAD, CREATE, CD do .,.. stuff
 1.38	 egb di/t sorts by mod time
 1.39	 SDG di/f improvements
 1.40	 SDG fix di/s (was removing files with duplicate times)
 	 SDG better error report from mount
 1.41	 SDG make clear skip directories, delete properly remove them,
	     create no longer creates names with slashes
 1.42	 SDG still try to delete <no access> files
 1.43	 SDG symbolic link
 1.44	 SDG shorten di/t format, delete dir's with '/' in name
 1.45	 SDG oops, broke btlstat() so that de dir got 214
 1.46	 SDG fixed clear adding garbage fields
 1.47	 SDG fix alignment in timestamp dump
 1.48	 SDG make du/e display uncompressed EBCDIC records
 1.49	 SDG fix starting record number for dump
 1.50	 SDG use new STAT name feature of kernel
 1.50a	 SDG report more errors for clear
 1.51	 SDG /d option on AR
 1.52	 SDG fix 219 error on umount
 1.53	 SDG improve ability of load to continue after errors
 1.54	 SDG fix mount command using wrong rlen
 1.55	 SDG show server version and compute Std Dev of I/O count / tran
*/
const char btas_version[] = "BTAS/X utility version 1.55";
