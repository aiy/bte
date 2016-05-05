BTE_CMD=../src/bte

echo "bad xml"
if $BTE_CMD test_bad_xml.xml ; then
	echo "failed: bad xml"
	exit 1
fi
echo "ok bad xml"

echo "not bt"
if $BTE_CMD test_not_bt.xml ; then
	echo "failed: not bt"
	exit 1
fi
echo "ok not bt"

echo "empty"
if ! $BTE_CMD test_empty_bt.xml ; then
	echo "failed: empty"
	exit 1
fi
echo "ok empty"

echo "one ok action"
if ! r=`$BTE_CMD test_one_ok_action_bt.xml 2>&1` ; then
	echo "failed: one ok action"
	exit 1
fi

