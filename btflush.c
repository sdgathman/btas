#include <btas.h>
#include <bterr.h>

main(int argc,char **argv) {
  int secs = 0;
  if (argc == 2)
    secs = atoi(argv[1]);
  if (!secs)
    secs = 30;
  for (;;) {
    int t = secs;
    int rc = btsync();
    if (rc == 298) break;
    if (rc == BTERBUSY)
      t = 2;
    sleep(t);
  }
  return 0;
}
