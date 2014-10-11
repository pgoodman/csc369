
catch signal SIGSEGV
commands
printf "BAD: Caught segmentation fault!\n"
thread apply all bt
q
end

catch signal SIGALRM
commands
printf "BAD: Program timed out\n"
thread apply all bt
q
end

catch signal SIGABRT
commands
printf "BAD: Program aborted. Likely assertion failure.\n"
thread apply all bt
q
end

r
q
