#!/bin/sh
exec >>/var/log/btas.log 2>&1 </dev/null
/bms/bin/btserve -b 2048 -c 2000000 /btdata/ROOT
#/bms/bin/btutil 'mo /btdata/ABI /edx/ABI' 
