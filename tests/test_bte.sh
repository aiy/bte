
echo "testing root"
if ! sh test_root_bte.sh ; then
	echo "root failed"
	exit 1
fi

echo "testing action"
if ! sh test_action_bte.sh ; then
	echo "action failed"
	exit 1
fi

echo "testing sequence"
if ! sh test_seq_bte.sh ; then
	echo "seq failed"
	exit 1
fi

echo "testing select"
if ! sh test_sel_bte.sh ; then
	echo "sel failed"
	exit 1
fi

echo "testing decorator succeeder"
if ! sh test_decorator_succeeder_bte.sh ; then
	echo "decorator succeeder failed"
	exit 1
fi

