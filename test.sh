#!/bin/sh

export BTSERVE=T
export LD_LIBRARY_PATH=lib

testBigExtent() {
  #startSkipping
  rm -f test.bt
  ./btstop 2>/dev/null
  # filesystem near 0xFFFFFF block limit
  assertTrue "btinit" "./btinit -f -b 2048 -e 16777210 test.bt"
  assertTrue "Server startup failed" "./btserve -b 2048 test.bt 2>test.log"
  assertTrue "util/btutil 'cr tmp'"
  assertTrue "testcisam" "cisam/testcisam >>test.log 2>&1"
  assertTrue "testlib" "lib/testlib >>test.log 2>&1"
  assertTrue "btsync" "util/btutil sy"
  assertTrue "btstop" "./btstop"
  # now see if btfs can be opened again
  assertTrue "Server startup failed" "./btserve -b 2048 test.bt 2>>test.log"
  lastline=`tail -1 test.log`
  assertEquals "Remount failed: $lastline" "test.bt mounted on /" "$lastline"
  assertTrue "./btstop"
}

testMountLimit() {
  rm -f test.bt
  ./btstop 2>/dev/null
  assertTrue "btinit" "./btinit -f -b 2048 -e 16777210 test.bt"
  for i in 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21; do
    cp test.bt test$i.bt
  done
  assertTrue "Server startup failed" "./btserve -b 2048 test.bt 2>test.log"
  export BTASDIR=/
  for i in 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21; do
    assertTrue "btutil cr mnt$i" "util/btutil 'cr mnt$i'"
    assertTrue "btutil mo test$i.bt /mnt$i" "util/btutil 'mo test$i.bt /mnt$i'"
  done
  assertTrue "util/btutil df>>test.log"
  lastline=`tail -1 test.log`
  assertEquals "mount failed: $lastline" "test21.bt 16777211 used" "$lastline"
  assertTrue "btstop" "./btstop"
  for i in 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21; do
    rm test$i.bt
  done
}

testFSLimit() {
  startSkipping	# doesn't work yet
  rm -f test.bt
  # filesystem near 2048 GiB ext3 filesystem limit
  assertTrue "btinit" "./btinit -f -b 2048 -e 1073741810 test.bt"
  assertTrue "Server startup failed" "./btserve -b 2048 test.bt 2>test.log"
  assertTrue "btutil 'cr tmp'" 
  assertTrue "testcisam" "cisam/testcisam 2>>test.log"
  assertTrue "btsync" "btutil sy"
  assertTrue "btstop" "./btstop"
  # now see if btfs can be opened again
  assertTrue "Server startup failed" "./btserve -b 2048 test.bt 2>>test.log"
  lastline=`tail -1 test.log`
  assertEquals "Remount failed: $lastline" "test.bt mounted on /" "$lastline"
  assertTrue "./btstop"
}

. ./shunit2

test "${__shunit_testsFailed}" -eq 0 || exit 1
