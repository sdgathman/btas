/*
	BTAS/2 utility functions
*/

#include <btas.h>
#include <stdlib.h>

#ifdef NDEBUG
#define STATIC static
#else
#define STATIC
#endif

int dir( void ), create( void ), getfcb( char * );
int mountfs( void ), unmountfs( void ), fsstat( void );
int dump( void ), delete( void );
int empty( void ), patch( void );
int archive( void ), load( void );
int copy( void ), change( void ), move( void ), linkbtfile( void );
int bunlock( void ), setdir( void ), pwdir( void ), pstat( void );
int getperm( int );
int help(void);
const char *permstr(short mode);
extern BTCB *b;		/* current directory */

#include "util.h"

extern const char btas_version[];

#ifndef __MSDOS__
extern int getperm( int );
#else
#define getperm(mask)	mask
#endif
