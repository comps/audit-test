#!/bin/bash
# =============================================================================
# (c) Copyright Hewlett-Packard Development Company, L.P., 2007
# Copyright (c) 2014 Red Hat, Inc. All rights reserved.
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of version 2 the GNU General Public License as
#   published by the Free Software Foundation.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
# =============================================================================
#
# It is important to note that prior to running the tests below the
# system must be configured using the configuration templates in the
# "network/system" directory as directed by the test plan.  Failure to
# configure the system correctly will result in test failures.
#
## Tests: xinetd, systemd
## PROGRAM:     xinetd/systemd
## PURPOSE:
## Verify that the xinetd/systemd daemon correctly runs child servers at the
## same sensitivity label as incoming connections when labeled networking is
## used. For xinetd the "LABELED" option must be set for the service. For
## systemd it is required to have SELinuxContextFromNet option set to yes in
## the socket file. This test verifies not only that the child service reports
## that it is running at the correct sensitivity label but that the audit
## records indicate the same sensitivity label.  The test procedure is as
## follows:
##  1. Connect to the test driver at a specific sensitivity label
##  2. Verify the reported sensitivity label matches the specified label and
##     that the audit records are correct
## TESTCASE:    connect to a xinetd/systemd controlled server at "s3"
## TESTCASE:    connect to a xinetd/systemd controlled server at "s4"
## TESTCASE:    connect to a xinetd/systemd controlled server at "s5"
## TESTCASE:    connect to a xinetd/systemd controlled server at "s5:c1"
## TESTCASE:    connect to a xinetd/systemd controlled server at "s5:c1.c5"
##
## 
## Test: sshd
## PROGRAM:     sshd
## PURPOSE:     see sshd_test function documentation

source testcase.bash || exit 2
source tp_ssh_functions.bash || exit 2

# be verbose
set -x

######################################################################
# global variables
######################################################################

######################################################################
# functions
######################################################################

#
# labeled_net - Test the labeled networking functionality of xinetd
#               and systemd
#
# INPUT
# $1 : MLS sensitivity label
#
# OUTPUT
# none
#
# DESCRIPTION
# This function tests xinetd's/systemd's ability to spawn child processes/servers with
# the same sensitivity label as the incoming connection.  This function
# connects to a local test driver spawned through xinetd/systemd; the test driver
# reports back to the client the sensitivity label at which it is running.
# In addition to comparing the actual test driver's sensitivity label with the
# requested sensitivity label this function also checks the audit log to ensure
# that the test driver was in fact spawned with the correct sensitivity label.
# If either the test driver or the audit records indicates a sensitivity label
# which does not match with what was requested the test fails and calls the
# exit_fail() function to indicate failure.
#
function labeled_test {
    declare log_mark
    declare subj=$1 rem_subj
    declare cmd_str="getcon:mls;"
    declare port=$2

    # determine the netcat variant
    if which nc6 >& /dev/null; then
        cmd_nc="nc6 --idle-timeout=1 -w 3 "
    elif which nc >& /dev/null; then
        cmd_nc="nc -w 3 "
    else
        die "error: netcat not installed"
    fi

    # mark the log for augrok later
    log_mark=$(stat -c %s $audit_log)

    # connect through xinetd/systemd
    rem_subj="$(runcon -t lspp_test_netlabel_t -l $subj \
	        $cmd_nc 127.0.0.1 $port <<< $cmd_str)"
    [[ $? != 0 ]] && exit_error "unable to connect to localhost"

    # verify label
    [[ "$rem_subj" != "$subj" ]] && exit_fail "labels do not match"

    # verify audit record
    augrok --seek=$log_mark type==SYSCALL success=yes \
	syscall=$(ausyscall execve | awk '{print $2}') \
	comm=lblnet_tst_serv \
	subj=system_u:system_r:lspp_harness_t:$subj || \
	exit_fail "missing audit record"
}

#
# This test exercises via localhost:
#
# 1. Socket activated, labeled networking aware sshd spawned by systemd
# on port 222. The security level of the logged in user must correspond
# to the security level of the incomming connection. It cannot be overriden.
# Note that this test scenario requires "allow init_t lo_netif_t:netif ingress;"
# SELinux rule to work - included # via lspp_test policy. This policy is
# not included in the default policy as it is not an usual scenario and is
# useful only for testing.
#
# 2. Login at different security level via "Traditional" SSH instance running
# on port 22. This is not related to labeled networking in any way, but is really
# close in testing scenario thus is included here.
#
sshd_test() {
    local SSH_MLS_PORT=222
    local SSH_PORT=22
    local SUSER="ssh_mls_user"
    local SOUT=

    # check if systemd listening on port 222
    ss -tlnp | grep -q "$SSH_MLS_PORT.*systemd" || \
        exit_error "Systemd not listening on port $SSH_MLS_PORT"

    # make sure we cleanup netlabel config on the end
    prepend_cleanup "systemctl restart netlabel"

    # faillock cleanup
    prepend_cleanup "faillock --reset --user $SUSER"

    # add test user
    useradd -G wheel $SUSER || exit_error "Error adding user $SUSER"
    prepend_cleanup "userdel -r $SUSER"
    echo "$SUSER:$PASSWD" | chpasswd
    semanage login -a -s staff_u -r s0-s10 $SUSER || exit_error \
        "Error adding selinux user $SUSER"
    prepend_cleanup "semanage login -d $SUSER"

    # test scenario 1. - systemd spawned sshd with labeling network
    for level in s3 s8 s10; do
        # setup labelling from localhost
        netlabelctl unlbl add default address:127.0.0.1/32 \
            label:"system_u:system_r:init_t:$level"

        # try to connect via ssh
        SOUT=$(ssh_mls_cmd_user "id -Z" $SUSER "-p 222")
        grep -q "staff_u:staff_r:staff_t:$level" <<< "$SOUT" || exit_fail \
            "Expected context 'staff_u:staff_r:staff_t:$level' not found"

        # cleanup netlabel rules
        netlabelctl unlbl del default address:127.0.0.1/32 \
label:\"system_u:system_r:init_t:$level\"
    done

    # test if security level cannot be overriden
    netlabelctl unlbl add default address:127.0.0.1/32 \
        label:"system_u:system_r:init_t:s10"

    # try to connect via ssh with overriden security level
    SOUT=$(ssh_mls_cmd_user "id -Z" ${SUSER}/staff_r/s3 "-p 222")
    grep -q "staff_u:staff_r:staff_t:$level" <<< "$SOUT" || \
        exit_fail "Expected context 'staff_u:staff_r:staff_t:s10' not found"

    # cleanup
    netlabelctl unlbl del default address:127.0.0.1/32 \
label:\"system_u:system_r:init_t:s10\"

    # test scenario 2. - connect with specified security level to "ordinary"
    # ssh daemon
    for level in s3 s8 s10; do
        SOUT=$(ssh_mls_cmd_user "id -Z" ${SUSER}/staff_r/$level)
        grep -q "staff_u:staff_r:staff_t:$level" <<< "$SOUT" || exit_fail \
            "Expected context 'staff_u:staff_r:staff_t:$level' not found"
    done

    # try to login with invalid security level
    SOUT=$(ssh_mls_cmd_user "id -Z" ${SUSER}/staff_r/s11)
    grep -q "staff_u:staff_r:staff_t:s11" <<< "$SOUT" && exit_fail \
        "Unexpected login with invalid security level s11"
}

#
# setup_systemd_labeled - create testing service in port 4002
#
setup_systemd_labeled() {
    prepend_cleanup "rm -f /etc/systemd/system/labeled.socket"
    prepend_cleanup "rm -f /etc/systemd/system/labeled@.service"
    prepend_cleanup "systemctl stop labeled.socket"

    cat > /etc/systemd/system/labeled.socket << EOF
[Unit]
Description=lbltest

[Socket]
ListenStream=4002
Accept=yes
SELinuxContextFromNet=true

[Install]
WantedBy=sockets.target
EOF

    cat > /etc/systemd/system/labeled\@.service << EOF
[Unit]
Description=lbltest

[Service]
LimitCORE=infinity
ExecStart=-/usr/local/eal4_testing/audit-test/utils/network-server/lblnet_tst_server -i -l /var/log/lblnet_tst_server.log -f /var/run/lblnet_tst_server6.pid -vv
StandardInput=socket
StandardError=journal
EOF

    systemctl daemon-reload
    systemctl start labeled.socket
}

#
# setup audit rules for systemd/xinetd tests
#
setup_audit_rules() {
    auditctl -a entry,always -S execve
    prepend_cleanup "auditctl -d entry,always -S execve"
}

case "$1" in
    systemd)
        setup_audit_rules
        setup_systemd_labeled
        port=4002
        ;;
    xinetd)
        setup_audit_rules
        port=4201
        ;;
    sshd)
        disable_ssh_strong_rng 
        sshd_test
        exit_pass
        ;;
    *)
        exit_error "Unknown test $1"
        ;;
esac

# run the test with various labels
for iter in "s3" "s4" "s5" "s5:c1" "s5:c1.c5"; do
    labeled_test $iter $port
done

# if we made it this far everything is okay
exit_pass

# vim: ts=4 sw=4 sts=4 ft=sh et ai:
