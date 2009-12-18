#!/bin/sh
usage() {
cat <<EOF
Usage: btlx filename[.idx] {filename[.idx] ...}
Description: Only lists the files in the specified filename.idx file.
Example:
  btcd /edx/AIRPEX/ISF
  btlx SHIP

Note: If you do not include the ".idx" in the filename it is assumed and automatically added.
EOF
}

test $# -eq 0 && usage && exit 1

opts=

while test $# -gt 0
do case "$1" in
-*) opts="$ops $1"; shift;;
*) break;;
esac
done


for i
do
  file=${i%%.idx}
  btl $opts $(btutil "du $file.idx"|cut -c0-18)
done
