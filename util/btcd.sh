btcd() {
case "$1" in
"")	;;
*)	if dir=`/usr/libexec/btas/btpwd "$1"`
	then export BTASDIR=$dir
	fi
	;;
esac
echo $BTASDIR
}
export MONEY=NEW
