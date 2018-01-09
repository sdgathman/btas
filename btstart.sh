#!/bin/sh
export MONEY=NEW

FSTAB="/etc/btas/btfstab$BTSERVE"
BASEDIR="/var/lib/btas"

/usr/libexec/btas/btstat >/dev/null && {
 echo "Server $BTSERVE already running."
 exit 1
}

test -s "${FSTAB}" || {
  echo "Missing ${FSTAB}"
  exit 1
}
umask 002
exec >>"/var/log/btas/btserve$BTSERVE.log" 2>&1 </dev/null

blksiz=1024
cache=2000000
rootfs=
while read fs mnt tb back; do
  case "$fs" in
  "#"*) continue;;
  esac
  case "$mnt" in
  "cache")	cache="$tb"; continue;;
  "/")		rootfs="$fs";;
  esac
  [ "$tb" -gt "$blksiz" ] && blksiz="$tb"
done <"${FSTAB}"

cd "$BASEDIR"
/usr/libexec/btas/btserve -b "${blksiz}" -c "${cache}" "${rootfs}"

while read fs mnt tb back; do
  case "$fs" in
  "#"*) continue;;
  esac
  case "$mnt" in
  "cache")	continue;;
  "/")		continue;;
  esac
  /usr/libexec/btas/btutil "mo $fs $mnt"
done <"${FSTAB}"

# Log current state
/usr/libexec/btas/btutil df

# Clear lock files
tmplock=/tmp/btstart.$$
for locklist in $(ls /etc/btas/clrlock.d/*)
do
  cat $locklist | while read lockfile 
  do
    case "$lockfile" in
    *LOCK*) # Sanity check - only clear files with LOCK in their name
      /usr/libexec/btas/btutil "du $lockfile" >$tmplock
      if test -s $tmplock
      then
        if fgrep -q 'Fatal error 212' $tmplock
        then
          : # echo "Ignore missing $lockfile"
        else
          echo "Clearing $lockfile"
          cat $tmplock
          /usr/libexec/btas/btutil "di $lockfile" "cl $lockfile"
        fi
      fi
      ;;
    esac
  done
done
rm -f $tmplock
