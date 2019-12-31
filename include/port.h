/*
	Edit this file with hacks needed to port between
	various flavours of 'C'
*/

#ifndef MORE
#define cdecl	/**/		/* only TURBOC groks these */
#define pascal 	/**/
#define far	/**/
#define near	/**/
#define huge	/**/
#define MORE	...
#define PTR	void *
#define NORETURN __attribute__ ((__noreturn__))
#define GOTO    void

#ifndef O_BINARY
#define O_BINARY 0
#define O_TEXT 0
#define _read read
#define _write write
#define _open open
#define _close close
#endif	/* O_BINARY */

#endif
