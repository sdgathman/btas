#!/bin/sh

export BTSERVE=T

testBigExtent() {
  rm -f test.bt
  assertTrue "btinit" "./btinit -f -b 2048 -e 16777210 test.bt"
  assertTrue "Server startup failed" "./btserve -b 2048 test.bt 2>test.log"
  assertTrue "btutil 'cr tmp'"
  assertTrue "testcisam" cisam/testcisam
  assertTrue "btsync" "btutil sy"
  assertTrue "btstop" "./btstop"
  # now see if btfs can be opened again
  assertTrue "Server startup failed" "./btserve -b 2048 test.bt 2>>test.log"
  lastline=`tail -1 test.log`
  assertEquals "Remount failed: $lastline" "test.bt mounted on /" "$lastline"
  assertTrue "./btstop"
}

. ./shunit2
