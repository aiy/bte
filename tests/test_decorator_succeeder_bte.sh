BTE_CMD=../src/bte

echo "empty succeeder decorator"
if ! $BTE_CMD test_decorator_succeeder_empty_bt.xml ; then
	echo "failed: empty succeeder decorator"
	exit 1
fi
echo "ok empty succeeder decorator"

echo "one ok action with succeeder decorator"
if ! r=`$BTE_CMD test_decorator_succeeder_one_ok_action_bt.xml 2>&1` ; then
	echo "failed: one ok succeeder decorator"
	exit 1
fi
m="Hi one seq"
if [ "$r" != "$m" ]; then
	echo "failed: output of one ok succeeder decorator"
	exit 1
fi
echo "ok one ok action succeeder decorator"

echo "one fail action with succeeder decorator"
if ! r=`$BTE_CMD test_decorator_succeeder_one_fail_action_bt.xml 2>&1`; then
  echo "failed: one fail action with succeeder decorator"
	exit 1
fi
m="sh: __fail_cmd__: command not found"
if [ "$r" != "$m" ]; then
  echo "failed: output of one fail action with succeeder decorator"
	exit 1
fi
echo "ok one fail action with succeeder decorator"

echo "two fail actions with succeeder decorator"
if ! r=`$BTE_CMD test_decorator_succeeder_two_fail_actions_bt.xml 2>&1`; then
  echo "failed: two fail actions with succeeder decorator"
	exit 1
fi
m="sh: __fail_cmd__0: command not found
sh: __fail_cmd__1: command not found"
if [ "$r" != "$m" ]; then
  echo "failed: output of two fail actions with succeeder decorator"
	exit 1
fi
echo "ok two fail actions with succeeder decorator"

echo "one ok one fail actions with succeeder decorator"
if ! r=`$BTE_CMD test_decorator_succeeder_one_ok_one_fail_actions_bt.xml 2>&1`; then
  echo "failed: one ok one fail actions with succeeder decorator"
	exit 1
fi
m="Hi one seq
sh: __fail_cmd__1: command not found"
if [ "$r" != "$m" ]; then
  echo "failed: output of one ok one fail actions with succeeder decorator"
	exit 1
fi
echo "ok one ok one fail actions with succeeder decorator"

<<REM

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
REM
