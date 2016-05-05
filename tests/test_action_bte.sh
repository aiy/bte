BTE_CMD=../src/bte

echo "one ok action"
if ! r=`$BTE_CMD test_one_ok_action_bt.xml 2>&1` ; then
	echo "failed: one ok action"
	exit 1
fi

m="Hi"
if [ "$r" != "$m" ]; then
	echo "failed: output of one ok action"
	exit 1
fi
echo "ok one ok action"

echo "one fail action"
if r=`$BTE_CMD test_one_fail_action_bt.xml 2>&1` ; then
	echo "failed: one fail action"
	exit 1
fi
m="sh: 1: __fail_cmd__: not found"
if [ "$r" != "$m" ]; then
	echo "failed: output of one fail action"
	exit 1
fi
echo "ok one fail action"

echo "two ok action"
if ! r=`$BTE_CMD test_two_ok_action_bt.xml 2>&1` ; then
	echo "failed: two ok action"
	exit 1
fi
m="Hi
Hi 1"
if [ "$r" != "$m" ]; then
	echo "failed: output of two ok action"
	exit 1
fi
echo "ok two ok action"

echo "two fail action"
if r=`$BTE_CMD test_two_fail_action_bt.xml 2>&1`; then
	echo "failed: two fail action"
	exit 1
fi
echo "ok two fail action"
m="sh: 1: __fail_cmd__: not found"
if [ "$r" != "$m" ]; then
	echo "failed: output of two fail action"
	exit 1
fi

echo "one ok one fail action"
if r=`$BTE_CMD test_one_ok_one_fail_action_bt.xml 2>&1` ; then
	echo "failed: one ok one fail action"
	exit 1
fi
m="Hi
sh: 1: __fail_cmd__: not found"
if [ "$r" != "$m" ]; then
	echo "failed: output of one ok one fail action"
	exit 1
fi
echo "ok one ok one fail action"

echo "one fail one ok action"
if r=`$BTE_CMD test_one_fail_one_ok_action_bt.xml 2>&1` ; then
	echo "failed: one fail one ok action"
	exit 1
fi
m="sh: 1: __fail_cmd__: not found"
if [ "$r" != "$m" ]; then
	echo "failed: output of one fail one ok action"
	exit 1
fi
echo "ok one fail one ok action"

echo "one big output action"
if r=`$BTE_CMD test_one_big_output_action_bt.xml > out 2>&1` ; then
  rm -f out
	echo "failed: one big action"
	exit 1
fi
if test $(stat -c%s out) -lt 255 ; then
  rm -f out
	echo "failed: one big action"
	exit 1
fi
rm -f out
echo "ok one big action"

