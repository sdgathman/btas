// Alarm class monitors time to auto-flush buffers
// and signals a shutdown for fatal signals.
#pragma interface

class Alarm {
  int limit;
  static int ticks, fatal, interval;
  static void handler(int);
public:
  Alarm();
  bool check();				// should we shutdown? (msgrcv err)
  void enable(int t);			// make sure alarm is ticking
  ~Alarm();
};
