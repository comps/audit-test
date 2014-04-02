#!/bin/bash

# This code sets up a generic user and then attempts to do an iptables
# modification operation. The test succeeds if the operation s denied.

source ../utils/functions.bash || exit 2

set -x

rc=0
export TEST_USER=testuser2
useradd -m -p usertest "$TEST_USER"
/bin/su - $TEST_USER -c "/sbin/iptables -I INPUT 1 -p tcp --dport 4100 -j DROP"
rc=$?
if [[ $rc -ne 0 ]]; then
    echo "operation not permitted, return code is $rc"
    killall -9 -u "$TEST_USER"
    userdel -rf "$TEST_USER" &>/dev/null
    ebtables -L
    exit_pass
else
    echo "test failed, ebtables operation permitted"
    killall -9 -u "$TEST_USER"
    userdel -rf "$TEST_USER" &>/dev/null
    ebtables -L
    exit_fail
fi
exit
