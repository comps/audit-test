#!/bin/bash
#
# this script counts all pass/fail/error results from rollup log and creates
# a total "summary" of all results
#
# it was originally written in awk, but due to severe language limitations,
# implementation diversity and several undocumented gensub() bugs, any slightly
# more complex string processing (ie. extracting run time) was impossible
#

PATH+="$PATH:$TOPDIR/utils"
source functions.bash || exit 2

pass=0
fail=0
error=0
runtime=0
while read line; do
    if [[ $line =~ ^TALLIED\ RESULTS ]]; then
        exit 0

    elif [[ $line =~ ^\ *([0-9]+)\ pass\ \([0-9]+%\)$ ]]; then
        pass=$(( pass + BASH_REMATCH[1] ))
    elif [[ $line =~ ^\ *([0-9]+)\ fail\ \([0-9]+%\)$ ]]; then
        fail=$(( fail + BASH_REMATCH[1] ))
    elif [[ $line =~ ^\ *([0-9]+)\ error\ \([0-9]+%\)$ ]]; then
        error=$(( error + BASH_REMATCH[1] ))

    elif [[ $line =~ ^\ *[0-9]+\ total\ \(in\ ([^\)]+)\)$ ]]; then
        runstr=${BASH_REMATCH[1]}
        runtime=$(( runtime + $(machine_time $runstr) ))
    fi
done

total=$(( pass + fail + error ))

if [[ $total -eq 0 ]]; then
    echo
    echo There are no results to be displayed
    echo
    exit 0
fi

runtime=$(human_time $runtime)

echo
echo TALLIED RESULTS
echo
printf "%4d pass (%d%%)\n" $pass $((pass*100/total))
printf "%4d fail (%d%%)\n" $fail $((fail*100/total))
printf "%4d error (%d%%)\n" $error $((error*100/total))
echo "------------------"
printf "%4d total (in %s)\n" $total $runtime
echo

# vim: sts=4 sw=4 et :
