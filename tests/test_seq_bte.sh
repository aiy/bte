BTE_CMD=../src/bte

echo "seq empty"
if ! $BTE_CMD test_seq_empty_bt.xml ; then
	echo "failed: seq empty"
	exit 1
fi
echo "ok seq empty"

echo "seq one ok action"
if ! r=`$BTE_CMD test_seq_one_ok_bt.xml 2>&1`; then
	echo "failed: seq one ok"
	exit 1
fi
m="Hi one seq"
if [ "$r" != "$m" ]; then
	echo "failed: seq one ok"
	exit 1
fi
echo "ok seq one ok action"

echo "seq one fail action"
if r=`$BTE_CMD test_seq_one_fail_bt.xml 2>&1` ; then
	echo "failed: seq one fail action"
	exit 1
fi
m="sh: __fail_cmd__: command not found"
if [ "$r" != "$m" ]; then
	echo "failed: output of seq one fail action"
	exit 1
fi
echo "ok seq one fail action"

echo "seq two ok action"
if ! r=`$BTE_CMD test_seq_two_ok_bt.xml 2>&1`; then
	echo "failed: seq two ok action"
	exit 1
fi
m="Hi one seq
Hi two seq"
if [ "$r" != "$m" ]; then
	echo "failed: output of seq two ok action"
	exit 1
fi
echo "ok seq two ok action"

echo "seq two fail action"
if r=`$BTE_CMD test_seq_two_fail_bt.xml 2>&1`; then
	echo "failed: seq two fail action"
	exit 1
fi
m="sh: __fail_cmd__0: command not found"
if [ "$r" != "$m" ]; then
	echo "failed: output of seq two fail action"
	exit 1
fi
echo "ok seq two fail action"

echo "seq one ok one fail action"
if r=`$BTE_CMD test_seq_one_ok_one_fail_bt.xml 2>&1`; then
	echo "failed: seq one ok one fail action"
	exit 1
fi
m="Hi one seq
sh: __fail_cmd__1: command not found"
if [ "$r" != "$m" ]; then
	echo "failed: output of seq one ok one fail action"
	exit 1
fi
echo "ok seq one ok one fail action"

echo "seq one fail one ok action"
if r=`$BTE_CMD test_seq_one_fail_one_ok_bt.xml 2>&1` ; then
	echo "failed: seq one fail one ok action"
	exit 1
fi
m="sh: __fail_cmd__0: command not found"
if [ "$r" != "$m" ]; then
	echo "failed: output of seq one fail one ok action"
	exit 1
fi
echo "ok seq one fail one ok action"

echo "seq two big ok action"
if ! r=`$BTE_CMD test_seq_two_big_ok_bt.xml 2>&1`; then
	echo "failed: seq two big ok action"
  rm -f *.zip
	exit 1
fi
rm -f *.zip
echo "ok seq two big ok action"

echo "seq 2 levels ok action"
if ! r=`$BTE_CMD test_seq_2l_ok_bt.xml 2>&1`; then
	echo "failed: seq 2l two ok action"
	exit 1
fi
m="Hi one seq 1l
Hi one seq 2l"
if [ "$r" != "$m" ]; then
	echo "failed: output of seq 2l two ok action"
	exit 1
fi
echo "ok seq 2l two ok action"

