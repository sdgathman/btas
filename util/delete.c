#include <stdio.h>
#include <string.h>
#include <errenv.h>
#include <bterr.h>
#include "btutil.h"
#include "btflds.h"

#define count(a)	(sizeof a / sizeof a[0])

int delete() {
  char *s = readtext("Filename to delete: ");
  struct btff ff;
  BTCB *b;
  if (!s) return 1;
  do {
    int rc;
    if (findfirst(s,&ff)) {
      (void)printf("%s: not found\n",s);
      continue;
    }
    b = btopen("",BTRDWR+BTDIROK,MAXREC);
    catch(rc)
      do {
	struct btstat st;
	struct btlevel fa;
	b->rlen = ff.b->rlen;
	(void)memcpy(b->lbuf,ff.b->lbuf,b->rlen);
	b->klen = strlen(b->lbuf) + 1;
	if (rc = btlstat(b,&st,&fa)) {
	  if (rc != BTERLINK)
	    printf("%s: <no access>\n",b->lbuf);
	  b->flags ^= BT_DIR;
	  if (btas(b,BTDELETE+NOKEY) == 0)
	    printf("%s: unlinked!\n",b->lbuf);
	  b->flags ^= BT_DIR;
	  continue;
	}
	if ((st.id.mode & BT_DIR) && !strchr(b->lbuf,'/')) {
	  rc = btrmdir(b->lbuf);
	  if (rc == 0)
	    printf("%s: directory removed!\n",b->lbuf);
	  else
	    printf("%s: directory not removed! (%d)\n",b->lbuf,rc);
	  continue;
	}
	if (btas(b,BTDELETE+NOKEY) == 0)
	  printf("%s: deleted!\n",b->lbuf);
      } while (findnext(&ff) == 0);
    enverr {
      finddone(&ff);
      (void)printf("Fatal error %d\n",rc);
    }
    envend
    (void)btclose(b);
  } while (s = readtext((char *)0));
  return 0;
}

int empty() {
  char *s = readtext("Filename to clear (delete all records): ");
  struct btstat st;
  struct btlevel fa;
  struct btff ff;
  BTCB *b;
  do {
    if (findfirst(s,&ff)) {
      (void)printf("%s: not found\n",s);
      continue;
    }
    b = btopen("",BTRDWR+4,MAXREC);
    envelope
      do {
	/* get permissions, check for directory */
	ff.b->klen = strlen(ff.b->lbuf);
	if (btlstat(ff.b,&st,&fa)) {
	  printf("%s: <no access>\n",ff.b->lbuf);
	  continue;
	}
	if (st.id.mode & BT_DIR) {
	  printf("%s: directory, not cleared.\n",ff.b->lbuf);
	  continue;
	}
	/* delete directory (and hence file) */
	b->klen = b->rlen = ff.b->rlen;
	(void)memcpy(b->lbuf,ff.b->lbuf,b->rlen);
	if (btas(b,BTDELETE+NOKEY) == 0) {
	  b->u.id = st.id;
	  b->rlen = ff.b->rlen;
	  (void)memcpy(b->lbuf,ff.b->lbuf,b->rlen);
	  b->klen = strlen(b->lbuf) + 1;
	  if (btas(b,BTCREATE+DUPKEY) == 0)
	    printf("%s: cleared!\n",ff.b->lbuf);
	  else
	    printf("%s: reincarnated!\n",b->lbuf);
	}
	else
	  printf("%s: vanished!\n",b->lbuf);
      } while (findnext(&ff) == 0);
    enverr {
      int rc = errno;
      finddone(&ff);
      (void)printf("Fatal error %d\n",rc);
    }
    envend
    (void)btclose(b);
  } while (s = readtext((char *)0));
  return 0;
}

int create() {
  char *s = readtext("Filename to create: ");
  int mode = 0666;
  BTCB *b = btopendir(s,BTWRONLY);
  b->rlen = b->klen + getfcb(b->lbuf+b->klen);
  if (b->klen == b->rlen) {		/* creating directory */
    int rc;
    btclose(b);
    mode |= BT_DIR+0111;
    switch (rc = btmkdir(s,mode)) {
    case BTERDUP:
      puts("btmkdir: Duplicate name");
      break;
    default:
      printf("btmkdir: failed (%d)\n",rc);
      break;
    case 0:
      break;
    }
    return 0;
  }
  b->u.id.user = getuid();
  b->u.id.group = getgid();
  b->u.id.mode  = mode;
  envelope
  if (btas(b,BTCREATE+DUPKEY))
    (void)puts("Duplicate name");
  envend
  btclose(b);
  return 0;
}

int getfcb(char *buf) {
  int f,klen;
  char *type, *bf;
  static struct { char *mnem, type, len, *desc; } typelist[] = {
    "C", BT_CHAR, 0, "Character",
    "D", BT_DATE, 0, "Date (if len: 2-# days since 1901, 4-julian days)",
    "X", BT_BIN, 0, "Binary Data",
    "T", BT_TIME, 0, "Time (if len: 4-secs since 1970, 6-julian secs",
    "N0", BT_NUM, 0, "Number with 0 decimal places",
    "N1", BT_NUM+1, 0, "Number with 1 decimal place",
    "N2", BT_NUM+2, 0, "Number with 2 decimal places",
    "N3", BT_NUM+3, 0, "Number with 3 decimal places",
    "N4", BT_NUM+4, 0, "Number with 4 decimal places",
    "N5", BT_NUM+5, 0, "Number with 5 decimal places",
    "N6", BT_NUM+6, 0, "Number with 6 decimal places",
    "N7", BT_NUM+7, 0, "Number with 7 decimal places",
    "N8", BT_NUM+8, 0, "Number with 8 decimal places",
    "N9", BT_NUM+9, 0, "Number with 9 decimal places",
    "FLONG", BT_NUM, 4,"4-byte number, no decimal places",
    "FSHORT", BT_NUM, 2,"2-byte number, no decimal places",
    "FMONEY", BT_NUM+2, 6,"6-byte number, 2 decimal places",
    "FDATE", BT_DATE, 2,"2-bytes representing # days since 1901",
    "FTIME", BT_TIME, 4,"4-bytes representing secs since 1970"
  };

  bf = buf;
  klen = getval((char *)0);
  if (!klen) return 0;
  *bf++ = klen;
  for (f = 0; f < MAXFLDS; ++f) {
    char *prompt;
    char pbuf[32];
    int i;
    (void)sprintf(pbuf,"Field #%d Type: ",f);

    prompt = pbuf;
    while (type = readtext(prompt)) {
      if (*type == '.') break;
      if (*type == '?') {
	puts("Enter one of the following:");
        for (i = 0; i < count(typelist); ++i)
	  printf("%s\t %s\n",typelist[i].mnem,typelist[i].desc);
	puts(".\t End Field Table Input");
	continue;
      }
      (void)strupr(type);
      for (i = 0; i < count(typelist); ++i) {
	if (strcmp(type,typelist[i].mnem) == 0) {
	  *bf++ = typelist[i].type;
	  if (typelist[i].len)
	    *bf++ = typelist[i].len;
	  else
	    *bf++ = getval("Length: ");
	  i = -1;
	  break;
	}
      }
      if (i < 0) break;
    }
    if (*type == '.') break;
  }
  *bf++ = 0;
  return bf - buf;
}
