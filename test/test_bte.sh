
if ! sh -x test_root_bte.sh ; then
	echo "root seq failed"
	exit 1
fi

if ! sh -x test_seq_bte.sh ; then
	echo "seq failed"
	exit 1
fi

