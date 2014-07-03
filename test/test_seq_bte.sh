BTE_CMD=../Debug/bte

echo "seq empty"
if ! $BTE_CMD test_seq_empty_bt.xml ; then
	echo "failed: seq empty"
	exit 1
fi
echo "ok seq empty"

echo "seq one ok action"
if ! $BTE_CMD test_seq_one_ok_bt.xml ; then
	echo "failed: seq one ok"
	exit 1
fi
echo "ok seq one ok action"

echo "seq one fail action"
if $BTE_CMD test_seq_one_fail_bt.xml ; then
	echo "failed: seq one fail action"
	exit 1
fi
echo "ok seq one fail action"


echo "seq two ok action"
if ! $BTE_CMD test_seq_two_ok_bt.xml ; then
	echo "failed: seq two ok action"
	exit 1
fi
echo "ok seq two ok action"

echo "seq two fail action"
if $BTE_CMD test_seq_two_fail_bt.xml ; then
	echo "failed: seq two fail action"
	exit 1
fi
echo "ok seq two fail action"

echo "seq one ok one fail action"
if $BTE_CMD test_seq_one_ok_one_fail_bt.xml ; then
	echo "failed: seq one ok one fail action"
	exit 1
fi
echo "ok seq one ok one fail action"

echo "seq one fail one ok action"
if $BTE_CMD test_seq_one_fail_one_ok_bt.xml ; then
	echo "failed: seq one fail one ok action"
	exit 1
fi
echo "ok seq one fail one ok action"
