#!/bin/sh

usage() {
cat <<EOF
Usage: btl [-x] [-t] [-a | -1 | -f field_list] [-o field_list] [pattern ...]
Options: 
 -x show sql statement before executing
 -b show sql statement before executing
 -t sort in descending order by mtime
 -a shows all fields
 -1 shows only the file name

 -f field_list is a list of comma seperated field names
    Available field names:
    name, root, mid, user, group, perm, uid, gid, mode, 
    mtime, atime, ctime, links, locks, recs, blks, rlen, klen 

 -o field_list order by specified fields 

 pattern is a filename you want to match 
 wildcard characters are % and _  where % matches any one or
 more characters and _ matches any one character

Example: btl -f name,rlen,klen,mtime INV%

EOF
exit
}

SELECT='select perm,user,locks,recs,blks,substr(name,1,22) "Name",rlen,klen'
WILD="'%'"
SORT=""
SQLFLGS=""
DEFAULT=1

if [ $# -gt 0 ] ; then
DONE=0
while [ $# -gt 0 -a $DONE -eq 0 ] ; do
  case $1 in
  -x|-q|-b) SQLFLGS="$SQLFLGS $1" 
      shift;;
  -a) SELECT="select *" ;
      shift;;
  -1) SELECT="select name" ;
      SQLFLGS="$SQLFLGS -q";
      shift;;
  -t) SORT="order by mtime desc"
      if [ $DEFAULT -eq 1 ]
      then
      SELECT="select perm,timemask(mtime,'Nnn DD CCCCC  ') "'"Last Modified"'"\
	      ,locks,recs,substr(name,1,22) "'"Name"'",rlen,klen"
      fi
      shift;;
  -f) if [ $# -lt 2 ] ; then
	usage
      fi
      SELECT="select $2"; DEFAULT=0
      shift; shift;;
  -o) if [ $# -lt 2 ] ; then
	usage
      fi
      SORT="order by $2";
      shift; shift;;
  -?) usage ;;
  *)  DONE=1;;
  esac
done
fi

if [ $# -gt 0 ] ; then
  WILD="'$1'"
  shift
  while [ $# -gt 0 ] ; do
    WILD=$WILD" or name like '$1'"
    shift
  done
fi

/usr/libexec/btas/sql $SQLFLGS <<EOF
$H
$SELECT from ""
where name like $WILD
$SORT
;
EOF
