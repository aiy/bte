BTE_CMD=../src/bte

echo "sel empty"
if ! $BTE_CMD test_sel_empty_bt.xml ; then
	echo "failed: sel empty"
	exit 1
fi
echo "ok sel empty"

echo "sel one ok action"
if ! r=`$BTE_CMD test_sel_one_ok_bt.xml 2>&1` ; then
	echo "failed: sel one ok"
	exit 1
fi
m="Hi one seq"
if [ "$r" != "$m" ]; then
	echo "failed: output of sel one ok"
	exit 1
fi
echo "ok sel one ok action"

echo "not sel one fail action"
if r=`$BTE_CMD test_sel_one_fail_bt.xml 2>&1`; then
	echo "failed: not sel one fail action"
	exit 1
fi
m="sh: __fail_cmd__: command not found"
if [ "$r" != "$m" ]; then
	echo "failed: output of not sel one fail action"
	exit 1
fi
echo "not ok sel one fail action"

echo "sel one from two ok action"
if ! r=`$BTE_CMD test_sel_from_two_ok_bt.xml 2>&1` ; then
	echo "failed: sel one from two ok action"
	exit 1
fi
m="Hi one sel"
if [ "$r" != "$m" ]; then
	echo "failed: output of sel one from two ok action"
	exit 1
fi
echo "ok sel one from two ok action"

echo "sel two fail action"
if r=`$BTE_CMD test_sel_two_fail_bt.xml 2>&1` ; then
	echo "failed: sel two fail action"
	exit 1
fi
m="sh: __fail_cmd__0: command not found
sh: __fail_cmd__1: command not found"
if [ "$r" != "$m" ]; then
	echo "failed: output of sel two fail action"
	exit 1
fi
echo "ok sel two fail action"

echo "sel one ok one fail action"
if ! r=`$BTE_CMD test_sel_one_ok_one_fail_bt.xml 2>&1` ; then
	echo "failed: sel one ok one fail action"
	exit 1
fi
m="Hi one seq"
if [ "$r" != "$m" ]; then
	echo "failed: output of sel one ok one fail action"
	exit 1
fi
echo "ok sel one ok one fail action"

echo "sel one fail one ok action"
if ! r=`$BTE_CMD test_sel_one_fail_one_ok_bt.xml 2>&1` ; then
	echo "failed: sel one fail one ok action"
	exit 1
fi
m="sh: __fail_cmd__0: command not found
Hi one seq"
if [ "$r" != "$m" ]; then
	echo "failed: output of sel one fail one ok action"
	exit 1
fi
echo "ok sel one fail one ok action"

echo "sel 2 levels ok action"
if ! r=`$BTE_CMD test_sel_2l_ok_bt.xml 2>&1` ; then
	echo "failed: sel 2l two ok action"
	exit 1
fi
m="Hi one seq 2l"
if [ "$r" != "$m" ]; then
	echo "failed: output of sel 2l two ok action"
	exit 1
fi
echo "ok sel 2l two ok action"

