BTE_CMD=../Debug/bte

echo "empty"
if ! $BTE_CMD test_empty_bt.xml ; then
	echo "failed: empty"
	exit 1
fi
echo "ok empty"

echo "one ok action"
if ! $BTE_CMD test_one_ok_action_bt.xml ; then
	echo "failed: one ok action"
	exit 1
fi
echo "ok one ok action"

echo "one fail action"
if $BTE_CMD test_one_fail_action_bt.xml ; then
	echo "failed: one fail action"
	exit 1
fi
echo "ok one fail action"

echo "two ok action"
if ! $BTE_CMD test_two_ok_action_bt.xml ; then
	echo "failed: two ok action"
	exit 1
fi
echo "ok two ok action"

echo "two fail action"
if $BTE_CMD test_two_fail_action_bt.xml ; then
	echo "failed: two fail action"
	exit 1
fi
echo "ok two fail action"

echo "one ok one fail action"
if $BTE_CMD test_one_ok_one_fail_action_bt.xml ; then
	echo "failed: one ok one fail action"
	exit 1
fi
echo "ok one ok one fail action"

echo "one fail one ok action"
if $BTE_CMD test_one_fail_one_ok_action_bt.xml ; then
	echo "failed: one fail one ok action"
	exit 1
fi
echo "ok one fail one ok action"


