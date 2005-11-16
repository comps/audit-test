#!/usr/bin/awk -f

BEGIN {
    "/bin/pwd" | getline cwd
}

{ print }

/^make.*Entering directory/ {
    dir = substr($NF, length(cwd)+2)
    dir = substr(dir, 1, length(dir)-1)
}

/^ *[0-9]+ pass \([0-9]+%\)$/ { pass += $1 }
/^ *[0-9]+ fail \([0-9]+%\)$/ { fail += $1 }
/^ *[0-9]+ error \([0-9]+%\)$/ { error += $1 }

END {
    total = pass + fail + error
    print ""
    print "TALLIED RESULTS"
    print ""
    printf "%4d pass (%d%%)\n", pass, pass*100/total
    printf "%4d fail (%d%%)\n", fail, fail*100/total
    printf "%4d error (%d%%)\n", error, error*100/total
    print "------------------"
    printf "%4d total\n", total
    print ""
}
