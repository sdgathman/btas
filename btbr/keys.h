/*	This module contains key and coresponding function numbers
	to be used with termcaps and BMS full screen functions	*/
#ifndef KEYSH		/* prevent duplicate declarations */
#define KEYSH		/* added 2/16/85  jls */
#define OFFSET		255   /* offset from std ascii values for extended
				 chars */

#include <keyboard.h>

#define	TABCH		"TB"
#define	TAB		KEY_TAB
#define	BACKTABCH	"kB"
#define	BACKTAB		KEY_BTAB
#define	BACKSPCH	"kb"
#define	BACKSPACE	KEY_BACKSPACE
#define	UPCH		"ku"
#define	UP		KEY_UP
#define	DNCH		"kd"
#define	DN		KEY_DOWN
#define	LEFTCH		"kl"
#define	LEFT		KEY_LEFT
#define	RIGHTCH		"kr"
#define	RIGHT		KEY_RIGHT
#define	HOMECH		"kh"
#define	HOME		KEY_HOME
#define	ENDCH		"@7"
#define	END		KEY_END
#define	TERMCH		"TM"
/* #define	TERMINATE	4 */
/* #define	WORDLCH		"WL" */
#define	WORDLEFT	KEY_PREVIOUS
/* #define	WORDRCH		"WR" */
#define	WORDRIGHT	KEY_NEXT 
#define	RETURNCH	"@8"	/* "RT" */
#define	RETURN		KEY_ENTER
#define NEWLINECH	"kH"
#define NEWLINE		KEY_LL
#define INQCH		F2CH
#define INQ		KEY_F(2)
#define HELPCH		F1CH
#define HELP		KEY_F(1)
#define REFRESHCH	"&2"	/* "RF" */
#define REFRESH		KEY_REFRESH
#define ERASECH		"kE"	/* "ce" */
#define ERASE		KEY_EOL
#define	F1CH		"k1"
#define	F1		KEY_F(1)
#define	F2CH		"k2"
#define	F2		KEY_F(2)
#define	F3CH		"k3"
#define	F3		KEY_F(3)
#define	F4CH		"k4"
#define	F4		KEY_F(4)
#define	F5CH		"k5"
#define	F5		KEY_F(5)
#define	F6CH		"k6"
#define	F6		KEY_F(6)
#define	F7CH		"k7"
#define	F7		KEY_F(7)
#define	F8CH		"k8"
#define	F8		KEY_F(8)
#define	F9CH		"k9"
#define	F9		KEY_F(9)
#define	F10CH		"k0"
#define	F10		KEY_F(10)
#define CTRLACH		"CA"
/* #define CTRLA		1 */
/* #define	CTRLBCH		"CB" */
/* #define	CTRLB	2 */
/* #define	CTRLDCH	"CD" */
/* #define	CTRLD	4 */
#define PRTCH	"%9"
#define PRT	KEY_PRINT

/* Added 2/6/85 jls to allow for Shifted Function key defs */
#define SF1CH		"F1"
#define SF1		84+OFFSET
#define SF2CH		"F2"
#define SF2		SF1+1
#define SF3CH		"F3"
#define SF3		SF1+2
#define SF4CH		"F4"
#define SF4		SF1+3
#define SF5CH		"F5"
#define SF5		SF1+4
#define SF6CH		"F6"
#define SF6		SF1+5
#define SF7CH		"F7"
#define SF7		SF1+6
#define SF8CH		"F8"
#define SF8		SF1+7
#define SF9CH		"F9"
#define SF9		SF1+8
#define SF10CH		"FA"
#define SF10		SF1+9

/* added 3/5/85 jls for use with img program */
#define INSCH		"kI"	/* "IC" */
#define INS		KEY_IC
#define DELCH		"kD"	/* "DC" */
#define DEL		KEY_DC

#define	INSKEY		F1
#define	DELKEY		F2
#define	INSLINEKEY	F3
#define	DELLINEKEY	F4
#define	STATUSKEY	F10
#define	CENTERKEY	F5

#endif
extern short fskey;
extern int getkey();
