
if ! sh test_root_bte.sh ; then
	echo "root seq failed"
	exit 1
fi

if ! sh test_seq_bte.sh ; then
	echo "seq failed"
	exit 1
fi

if ! sh test_sel_bte.sh ; then
	echo "sel failed"
	exit 1
fi

