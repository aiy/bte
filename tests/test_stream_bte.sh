BTE_CMD=../src/bte

echo "stream open/close xml"
if ! r=`$BTE_CMD test_stream_open_close_bt.xml` ; then
	echo "failed: stream open close xml"
	exit 1
fi
echo "ok stream open close xml"

