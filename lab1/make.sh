for d in $(find ~/Downloads/csc369a1 -maxdepth 1 -type d); do
  if [ -d "$d/a1" ]; then
    student="${d##*/}"
    mkdir -p $d/../log/$student
    make -w -C $d/a1 clean all > $d/../log/$student/make.log 2>&1
  fi
done
