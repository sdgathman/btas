long getval( const char * );		/* get value with conditional prompt */
char *readtext( const char * );		/* get text with conditional prompt */
int question( const char * );		/* ask yes/no question */
char *gettoken(char *,const char *);	/* strtok w/ quote interpretation */
extern volatile int cancel;	/* cancel flag */
extern char *switch_char;	/* command options */
