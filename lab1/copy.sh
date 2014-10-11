for d in $(find ~/Downloads/csc369a1 -maxdepth 1 -type d); do
	cp test_*.c $d/a1
	cp test_*.cc $d/a1
	cp test_*.h $d/a1
	cp Makefile $d/a1
done
