btcd() {
case "$1" in
"")	;;
*)	if dir=`btpwd "$1"`
	then export BTASDIR=$dir
	fi
	;;
esac
echo $BTASDIR
}

