BTE_CMD=../src/bte

echo "test stream open/close"
if ! r=`$BTE_CMD test_stream_open_close_bt.xml` ; then
	echo "failed: test stream open close"
	exit 1
fi
echo "ok test stream open close"

echo "test stream expect"
if ! r=`$BTE_CMD test_stream_expect_bt.xml` ; then
	echo "failed: test stream expect"
	exit 1
fi
echo "ok test stream expect"

echo "test stream write"
if ! r=`$BTE_CMD test_stream_write_bt.xml` ; then
	echo "failed: test stream write"
	exit 1
fi
echo "ok test stream write"

echo "test stream expect command"
if ! r=`$BTE_CMD test_stream_write_shell_bt.xml` ; then
	echo "failed: test stream expect command"
	exit 1
fi
echo "ok test stream expect command"
