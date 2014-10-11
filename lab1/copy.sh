for d in $(find ~/Downloads/csc369a1 -maxdepth 1 -type d); do
  if [ -d "$d/a1" ]; then

    # If we don't have a `mymemory_opt.c`, then assume we have a `mymemory.c`
    # and make it into `mymemory_opt.c`.
    if [ ! -f "$d/a1/mymemory_opt.c" ]; then
      cp $d/a1/mymemory.c $d/a1/mymemory_opt.c > /dev/null 2>&1 ||:
    fi

    # Remove any unusually named files that might also define `mymalloc` and
    # `myfree`.
    rm $d/a1/*mymemory.c > /dev/null 2>&1 ||:

    # Remove their makefile(s) and any files that might clash with the tester.
    rm $d/a1/Makefile $d/a1/makefile $d/a1/test_* > /dev/null 2>&1 ||:

    # Remove any old log files.
    rm $d/a1/*.log > /dev/null 2>&1 ||:

    # Copy in the tester source and header files, and Makefile.
    cp test_*.c test_*.cc test_*.h Makefile $d/a1
  fi
done
