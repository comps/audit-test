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
#  Tested audisp-remote.conf option: disk_full_action
#  TOE is acting as server and remote client at the same time on unique ports.


source audisp-remote_functions.bash || exit 2

function print_available_blocks {
    printf "++++\navailable blocks: %s\n++++\n" `stat -f -c %a /var/log/audit`
}

function trigger_daemon_disk_full_action {
    local test_msg=${1:-"`create_user_test_msg`"}

    # Fill the local filesystem hosting audit.log, leaving 5k available
    # compensate for 64k page size on power
    if [[ $ARCH != "PPC" ]]; then
       fill_disk ${audit_log%/*} 5 || exit_error "fill_disk failed"
    else
       fill_disk ${audit_log%/*} 70 || exit_error "fill_disk failed"
    fi

    # each record is at least 80 bytes (based on empirical evidence), so writing
    # 65 records should always take us over (65 * 80 =~ 5k)
    # if powerpc it will take 875 80 byte records to get to the 70K
    if [[ $ARCH != "PPC" ]]; then
       write_local_records 65 || exit_error "write_local_records failed"
    else
       write_local_records 875 || exit_error "write_local_records failed"
    fi

    # Trigger action of the remote plugin so we can check for configured action.
    # Server should report full disk back to the plugin.
    send_audisp_remote_test_msg $test_msg
}

function check_plugin_disk_full_action {
    # Use global action if none provided explicitely
    remote_action=${1:-$action}
    case $remote_action in
        syslog)
            remote_check_$remote_action \
            "remote server's disk is full, disk full"
            ;;
        suspend)
            # Note: we are not testing resume from suspend on SIGUSR2
            remote_check_$remote_action \
            "suspending remote logging due to remote server's disk is full"
            ;;
        stop)
            search_syslog "remote logging stopping due to remote server's disk is full, disk full" && \
            remote_check_$remote_action "audisp-remote is exiting on stop request"
            ;;
        halt)
            search_syslog "remote logging halting system due to remote server's disk is full" && \
            remote_check_$remote_action
            ;;
        single)
            search_syslog "remote logging is switching system to single user mode due to remote server's disk is full" && \
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

configure_local_auditd_disk_full_action IGNORE
configure_local_plugin_disk_full_action

restart_auditd || exit_error "Failed to restart auditd"

# Make sure remote logging part works
test_msg="`create_user_test_msg`"
send_audisp_remote_test_msg $test_msg
check_received_test_msg


#
# Test
#

# Be verbose
set -x

trigger_daemon_disk_full_action $test_msg
check_plugin_disk_full_action || exit_fail

# If we got this far, we are good to PASS
exit_pass
