#!/bin/bash
###############################################################################
#   Copyright (c) 2011 Red Hat, Inc. All rights reserved.
#
#   This copyrighted material is made available to anyone wishing
#   to use, modify, copy, or redistribute it subject to the terms
#   and conditions of the GNU General Public License version 2.
#
#   This program is distributed in the hope that it will be
#   useful, but WITHOUT ANY WARRANTY; without even the implied
#   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#   PURPOSE. See the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public
#   License along with this program; if not, write to the Free
#   Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
#   Boston, MA 02110-1301, USA.
###############################################################################
#
# FMT_MTD.1(AUD-AT)
#
# AUTHOR: Eduard Benes <ebenes@email>
#
# DESCRIPTION:
#  Audisp remote logging plugin test for:
#  a) threshold of the audit trail when an action is performed;
#  b) action when the threshold is reached
#
#  Tested audisp-remote.conf option: remote_ending_action
#  TOE is acting as server and remote client at the same time on unique ports.


source audisp-remote_functions.bash || exit 2

function trigger_daemon_remote_ending_action {
    local test_msg=${1:-"`create_user_test_msg`"}

    # Simulate network failure
    # Prepere some records to pass through the audisp-remote plugin later
    write_local_records 50 || exit_error "write_local_records failed"

    # We need to simulate a slow network first and restore at exit.
    # The slow connection gives us enough time to simulate network error.
    prepend_cleanup "tc qdisc del dev lo root"
    tc qdisc add dev lo root netem delay 500ms

    # There is a change in behavior of audisp-remote plugin in 6.2 version.
    # If the input stream closes, we shutdown plugin and protect the queue.
    # Therefore we need to sleep and let the plugin send the data before
    # the pipe closes the fd plugin reads from.
    # See code around line audisp-remote.c:546 for details.
    sleep 5 &
    pid_sleep=$!
    ( tail --pid $pid_sleep -n50 -f /var/log/audit/audit.log | audisp-remote ) &
    # Give it some time to send few messages and then simulate remote ending
    sleep 2
    prepend_cleanup "start_auditd"
    stop_auditd
    sleep 3
    tc qdisc del dev lo root
    start_auditd
    sleep 2
    # Final sanity check after we restored the correct behaviour
    send_audisp_remote_test_msg $test_msg
}

function check_plugin_remote_ending_action {
    # Use global action if none provided explicitely
    remote_action=${1:-$action}
    case $remote_action in
        reconnect)
            remote_check_$remote_action \
            "remote logging disconnected due to remote server is going down, will attempt reconnection"
            ;;
        syslog)
            remote_check_$remote_action \
            "remote server is going down"
            ;;
        suspend)
            # Note: we are not testing resume from suspend on SIGUSR2
            remote_check_$remote_action \
            "suspending remote logging due to remote server is going down"
            ;;
        stop)
            remote_check_$remote_action \
            "audisp-remote is exiting on stop request"
            ;;
        halt)
            search_syslog "remote logging halting system due to remote server is going down" && \
            remote_check_$remote_action
            ;;
        single)
            search_syslog "remote logging is switching system to single user mode due to remote server is going down" && \
            remote_check_$remote_action
            ;;
        exec)
            remote_check_$remote_action
            ;;
        *)
            exit_error "Unsupported action: $remote_action " ;;
    esac
    return $?
}


#
# Configure
#

common_server_startup || exit_error "common_server_startup"
configure_local_audit_server
configure_local_audisp_remote

# There should be no need to configure server for any action
configure_local_auditd_remote_ending_action
configure_local_plugin_remote_ending_action

restart_auditd || exit_error "Failed to restart auditd"

# Make sure remote logging part works
test_msg="`create_user_test_msg`"
send_audisp_remote_test_msg $test_msg
check_received_test_msg

#
# Test
#

# prepend_cleanup print_audit_log_snippet

# Be verbose
set -x

trigger_daemon_remote_ending_action $test_msg
check_plugin_remote_ending_action || exit_fail

# If we got this far, we are good to PASS
exit_pass
