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
#  Tested audisp-remote.conf option: disk_low_action
#  Test uses TOE for both, server and remote client.


source audisp-remote_functions.bash || exit 2

function trigger_daemon_disk_low_action {
    # Make sure remote logging part works
    test_msg="`create_user_test_msg`"
    send_audisp_remote_test_msg $test_msg
    check_received_test_msg

    # Fill the local filesystem hosting audit.log, leaving 1MB + 5KB available
    fill_disk ${audit_log%/*} $((1024 + 5)) || exit_error "fill_disk failed"
    # each record is at least 80 bytes (based on empirical evidence), so writing
    # 200 records should always take us over (200 * 80 =~ 15k)
    write_local_records 200 || exit_error "write_local_records failed"

    # Trigger action of the remote plugin so we can check for configured action.
    # Server should report low disk space back to the plugin.
    test_msg="`create_user_test_msg`"
    send_audisp_remote_test_msg $test_msg
}

function check_plugin_disk_low_action {
    # Use global action if none provided explicitely
    remote_action=${1:-$action}
    case $remote_action in
        syslog)
            remote_check_$remote_action \
            "remote server is low on disk space,"
            ;;
        suspend)
            # Note: we are not testing resume from suspend on SIGUSR2
            remote_check_$remote_action \
            "suspending remote logging due to remote server is low on disk"
            ;;
        stop)
            search_syslog "remote logging stopping due to remote server is low on disk space," && \
            remote_check_$remote_action "audisp-remote is exiting on stop request"
            ;;
        halt)
            search_syslog "remote logging halting system due to remote server is low on disk space" && \
            remote_check_$remote_action
            ;;
        single)
            search_syslog "remote logging is switching system to single user mode due to remote server is low on disk space" && \
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

configure_local_auditd_disk_low_action IGNORE
if [ $action = exec ] ; then
    configure_local_plugin_disk_low_action
else
    configure_local_plugin_disk_low_action
fi

restart_auditd || exit_error "Failed to restart auditd"


#
# Test
#

# Be verbose
set -x

trigger_daemon_disk_low_action
check_plugin_disk_low_action || exit_fail

# If we got this far, we are good to PASS
exit_pass

