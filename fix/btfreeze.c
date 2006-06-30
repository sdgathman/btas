#include <stdio.h>
#include <btas.h>

main() {
  BTCB b;
  if (btas(&b,BTFLUSH + BTEXCL + LOCK)) {
    fputs("BTAS/X frozen for the duration of this process\n",stderr);
    pause();
    return 0;
  }
  return 1;
}
