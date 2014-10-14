for d in $(find ~/Downloads/csc369a1 -maxdepth 1 -type d); do
  if [ -f "$d/a1/a.out" ]; then
    student="${d##*/}"
    mkdir -p $d/../log/$student
    for t in $(find traces -maxdepth 1 -type f); do
      trace="${t##*/}"
      gdb -x gdb_script --quiet --args $d/a1/a.out $t > $d/../log/$student/$trace.log 2>&1
    done
  fi
done
