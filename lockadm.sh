#!/bin/sh

usage() {
cat <<EOF
Usage: lockadm [-r|-l] [aw|wr|ox|bb|abi|dom|awinv|oxinv ...]
Options:
  -l List Locks
  -r Remove Lock
EOF
}

umask=0
LOG=/usr/tmp/lockadm.log
BASEDIR=/edx/AIRPEX

# Selects for listing locks.
SELECT1=',l.#4 "Session",l.#6 "UserID",u.#4 "User Name",timemask(l.#7,'"'Nnn DD hh:mm:ss'"') "Time"'
WHERE1="l.#6 = u.#1"

SELECT2=',l.#5 "Session",l.#7 "UserID",u.#4 "User Name",timemask(l.#8,'"'Nnn DD hh:mm:ss'"') "Time"'
WHERE2="l.#7 = u.#1"


# FIXME: Add additional case statements for all modules.

# Initialize SELECT and FILE variables.
getselect() {
case "$1" in
aw)  FILE="$BASEDIR"'/AWB/WAYBILL.LOCK'
     SELKEY="pic(#1,'###')||'-'||pic(#2,'########')"
     SELLIST="$SELKEY"' "AWB#",l.#3 "Session"'
     WHERELIST="#1 > 0" # dummy where
     ;;
wr)  FILE="$BASEDIR"'/AWB/WHSE.RCPT.LOCK'
     SELKEY="pic(#1,'ZZZZZZZ#')"
     SELLIST='l.#1 "Receipt#"'"$SELECT1"
     WHERELIST="$WHERE1"
     ;;
awinv) FILE="$BASEDIR"'/AWB/INVOICE.LOCK'
     SELKEY="pic(#1,'ZZZZZZZ#')"
     SELLIST='l.#1 "Invoice#"'"$SELECT1"
     WHERELIST="$WHERE1"
     ;;
ox)  FILE="$BASEDIR"'/OX/LOG.LOCK'
     SELKEY="pic(#1,'ZZZZZZZ#')||#2"
     SELLIST="$SELKEY"' "File#"'"$SELECT2"
     WHERELIST="$WHERE2"
     ;;
esac
}

# Initialize KEY and WHERE variables.
getkey() {
case "$1" in
aw) echo -e "Enter AWB# (###-########): \c"
    read awb
    test -z "$awb" && exit 0
    awb1=${awb%%-*}
    awb2=${awb##*-}
    if test "$awb1" -gt 1000 -o "$awb2" -gt 99999999 2>/dev/null
    then
      echo "$awb is invalid"
      exit 1
    fi
    KEY="$awb"
    WHERE="where #1 = $awb1 and #2 = $awb2"
    ;;
wr) echo -e "Enter Warehouse Receipt#: \c"
    read wrno
    test -z "$wrno" && exit 0
    if test "$wrno" -gt 99999999 2>/dev/null
    then
      echo "$wrno is invalid"
      exit 1
    fi
    KEY="$wrno"
    WHERE="where #1 = $wrno"
    ;;
awi) echo -e "Enter AWB Invoice#: \c"
    read awino
    test -z "$awino" && exit 0
    if test "$awino" -gt 999999999 2>/dev/null
    then
      echo "$awino is invalid"
      exit 1
    fi
    KEY="$awino"
    WHERE="where #1 = $awino"
    ;;
ox) echo -e "Enter File#: \c"
    read oxno
    test -z "$oxno" && exit 0
    if ! [[ "$oxno" =~ '^[0-9]+$' ]]
    then
      echo "$oxno is not a number"
      exit 1
    fi
    if test "$oxno" -gt 99999999 2>/dev/null
    then
      echo "$oxno is invalid"
      exit 1
    fi
    KEY="$oxno"
    WHERE="where #1 = $oxno and #2 = '$oxnos'"
    ;;
esac
}

listlock() {
getselect "$1"

sql <<EOF
select $SELLIST
from "$FILE" l, outer "/edx/MENU.USER" u
where $WHERELIST
;
EOF
}

readlock() {
sql -q <<EOF
select $SELKEY from "$FILE" $WHERE;
EOF
}

removelock() {
echo $(date '+%F %T') $(id -u) $(id -u -n) remove "$FILE" "$KEY" >>$LOG
sql -q <<EOF
delete from "$FILE" $WHERE;
EOF
}

dellock() {
  getselect "$1"
  getkey "$1"
  found=$(readlock)

  if test -n "$found"
  then
    echo -e "Delete lock for $found? (Y/N) \c"
    read yn
    case "$yn" in
    Y|y) removelock;;
    esac
  else
    echo "$KEY is not locked"
  fi
}

test "$#" -lt 2 && usage && exit 1

case "$1" in
-l) listlock "$2" ;;
-r) dellock "$2" ;;
*) usage && exit 1 ;;
esac
