#include <stdio.h>
#include <stdlib.h>
#include <btas.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

main() {
  int btasreq, btasres;
  struct msqid_ds buf;
  const char *s = getenv("BTSERVE");
  long key;
  if (!s) s = "";
  key = BTASKEY + *s * 2;
  btasreq = msgget(key,0620);
  btasres = msgget(key+1,0640);
  if (btasreq == -1 && btasres == -1) {
    printf("Server%s not loaded\n",s);
    return 1;
  }
  printf("Server%s loaded\n",s);
  return 0;
}
