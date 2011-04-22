#!/bin/bash
FSTAB="/bms/etc/btfstab$BTSERVE"
test -d "/btsave$BTSERVE" || {
  echo "Backup directory /btsave$BTSERVE does not exist."
  exit 1
}
touch "/btsave$BTSERVE/backup" && rm "/btsave$BTSERVE/backup" || {
  echo "Backup directory /btsave$BTSERVE is not writable."
  exit 1
}

/bms/bin/btstop
while read fs mnt tb back; do
  case "$fs" in
  "#") continue;;
  esac
  [ -z "$back" ] && continue
  [ "$back" -eq 0 ] && continue
  [ "$mnt" = "cache" ] && continue
  /bms/bin/btsave "$fs" | gzip --rsyncable >"/btsave$BTSERVE/${fs##*/}.gz"
done <"${FSTAB}"
/bms/bin/btstart
