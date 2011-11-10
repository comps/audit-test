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
#  Tested audisp-remote.conf option: disk_error_action
#  TOE is acting as server and remote client at the same time on unique ports.


source audisp-remote_functions.bash || exit 2

function trigger_daemon_disk_error_action {
    local test_msg=${1:-"`create_user_test_msg`"}

    # Simulate disk error
    chcon system_u:object_r:games_data_t:s0 /var/log/audit/audit.log
    prepend_cleanup "restorecon /var/log/audit/audit.log"

    write_local_records 1 || exit_error "write_local_records failed"

    # Trigger action of the remote plugin so we can check for configured action.
    # Server should report error disk back to the plugin.
    send_audisp_remote_test_msg $test_msg
    sleep 1
}

function check_plugin_disk_error_action {
    # Use global action if none provided explicitely
    remote_action=${1:-$action}
    case $remote_action in
        syslog)
            remote_check_$remote_action \
            "remote server has a disk error, disk write error"
            ;;
        suspend)
            # Note: we are not testing resume from suspend on SIGUSR2
            remote_check_$remote_action \
            "suspending remote logging due to remote server has a disk error"
            ;;
        stop)
            remote_check_$remote_action \
            "remote logging stopping due to remote server has a disk error,"\
            "disk write error"
            ;;
        halt)
            search_syslog "remote logging halting system due to remote server has a disk error" && \
            remote_check_$remote_action
            ;;
        single)
            search_syslog "remote logging is switching system to single user mode due to remote server has a disk error" && \
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

selinuxenabled && grep 1 /selinux/enforce || exit_error "SELinux not in Enforcing"

common_server_startup || exit_error "common_server_startup"
configure_local_audit_server
configure_local_audisp_remote

configure_local_auditd_disk_error_action IGNORE
#configure_local_auditd_disk_error_action SUSPEND
configure_local_plugin_disk_error_action

restart_auditd || exit_error "Failed to restart auditd"

# Make sure remote logging part works
test_msg="`create_user_test_msg`"
send_audisp_remote_test_msg $test_msg
check_received_test_msg


#
# Test
#

#prepend_cleanup print_audit_log_snippet

# Be verbose
set -x

mark_syslog

trigger_daemon_disk_error_action $test_msg
check_plugin_disk_error_action || exit_fail

# Remove all messages generated by auditd during testing since last call
# of mark_syslog function
restore_syslog

# If we got this far, we are good to PASS
exit_pass
