#!/bin/sh
usage() {
cat <<EOF
Usage: btlc pattern
Description:
Lists btas filenames that match the pattern specified.
pattern is a filename you want to match 
wildcard characters are % and _  where % matches any one or
more characters and _ matches any one character
EOF
exit 1
}

test $# -eq 1 -a "$1" = '-?' && usage

WILD="'%'"

if [ $# -gt 0 ] ; then
  WILD="'$1'"
  shift
  while [ $# -gt 0 ] ; do
    WILD=$WILD" or name like '$1'"
    shift
  done
fi

sql -q <<EOF | pr -4 -w80 -t
select name from ""
where name like $WILD
;
EOF
