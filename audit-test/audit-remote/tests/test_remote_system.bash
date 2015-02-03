#!/bin/bash -x
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
# Testing of an audit remote logging functional requirements on isolated
# systems.
#
# AUTHOR: Eduard Benes <ebenes@redhat.com>
#
# DESCRIPTION:
#  Using network server as a remote system capable of running as audit remote
#  logging client or server. Actions on a remote system are operated via calls
#  to lblnet_tst_server listening on port 4000. New action has been  added
#  to lblnet_tst_server for this specific purpose.
#


source audisp-remote_functions.bash || exit 2

[ -z "$LBLNET_SVR_IPV4" ] && exit_error "Please, define LBLNET_SVR_IPV4"

# Test scenario to test for by calling corresponding function
scenario=$1
test_time_start="`date +'%x %T'`"
call_remote_function_seq=0
attempts_limit=5

# Redefine waiting for wait_for_cmd function (see functions.bash)
WAIT_FOR_SLEEP=2
WAIT_FOR_MAX=5

#
# Helper functions
#

call_remote_function() {
    local call_function="$1"
    local my_ip="$LOCAL_IPV4"
    # the mode variable will get here from audisp-remote_functions.bash
    echo "---- START [$call_remote_function_seq] call_remote_function($call_function) ----"
    /usr/bin/nc -v $LBLNET_SVR_IPV4 4000 <<< "remote_call:audit-remote.bash,$call_function,$mode,$my_ip;"
    echo "---- END [$call_remote_function_seq] call_remote_function($call_function)  ----"
    ((call_remote_function_seq+=1))
}

# System configuration

ping_netsrv_and_backup_configs() {
    # Make sure server is there waiting for us and backup audit configs
    /bin/ping -c 3 $LBLNET_SVR_IPV4 || \
        exit_error "Network server not reachable"
    append_cleanup "restart_service auditd"

    backup $auditd_conf
    backup $audisp_remote_conf
    backup $au_remote_conf
    backup $audispd_conf
}

# TOE is a client and netserver is acting as a server in this case
configure_toe_client() {
    ping_netsrv_and_backup_configs
    stop_service auditd || exit_error "stop_service auditd"

    # Configure to act as a client
    write_config -r "$auditd_conf" tcp_listen_port
    write_config -s "$auditd_conf" \
        name_format=user \
        name=REMOTE_LOGGING_CLIENT \
        dispatcher=/sbin/audispd \
        disp_qos=lossless

    write_config -s "$audispd_conf" \
        name_format=user \
        name=REMOTE_LOGGING_CLIENT

    write_config -s "$audisp_remote_conf" \
        remote_server=$LBLNET_SVR_IPV4 \
        remote_ending_action=reconnect \
        mode=$mode

    write_config -s "$au_remote_conf" \
        active=yes

    start_service auditd || exit_error "start_service auditd"
}

configure_toe_client_disk_action() {
    local full_action=IGNORE
    local error_action=IGNORE

    # Use specified action or default STOP
    case $1 in
      "full") full_action=${2:-STOP}
        ;;
      "error") error_action=${2:-STOP}
        ;;
      * ) exit_error "configure_toe_client_disk_action() $1_action is not known"
        ;;
    esac

    ping_netsrv_and_backup_configs
    stop_service auditd || exit_error "stop_service auditd"

    # Netserver is a server and TOE is a client for remote audit logging
    write_config -r "$auditd_conf" tcp_listen_port
    write_config -s "$auditd_conf" \
        admin_space_left_action=IGNORE \
        disk_full_action=IGNORE \
        disk_error_action=IGNORE \
        name_format=user \
        name=REMOTE_LOGGING_CLIENT \
        dispatcher=/sbin/audispd \
        disp_qos=lossless

    write_config -s "$audispd_conf" \
        name_format=user \
        name=REMOTE_LOGGING_CLIENT

    write_config -s "$audisp_remote_conf" \
        remote_server=$LBLNET_SVR_IPV4 \
        remote_ending_action=reconnect \
        mode=$mode \
        network_failure_action=IGNORE \
        generic_error_action=IGNORE \
        generic_warning_action=IGNORE \
        disk_full_action=$full_action \
        disk_error_action=$error_action \
        disk_low_action=IGNORE

    write_config -s "$au_remote_conf" \
        active=yes

    start_service auditd || exit_error "start_service auditd"
}


# TOE will act as a server and IP addr is in LOCAL_IPV4 from net profile
configure_toe_server() {
    local msg

    ping_netsrv_and_backup_configs
    stop_service auditd || exit_error "stop_service auditd"

    # Configure to act as a server
    write_config -s "$au_remote_conf" \
        active=no

    write_config -s "$auditd_conf" \
        tcp_listen_port=60 \
        name_format=user \
        name=REMOTE_LOGGING_SERVER \
        disp_qos=lossless

    write_config -s "$audispd_conf" \
        name_format=user \
        name=REMOTE_LOGGING_SERVER

    start_service auditd || exit_error "start_service auditd"

    # Make sure we are good to start testing
    msg="`create_user_test_msg`"
    ausearch -m USER -ts $test_time_start | grep "$msg" || \
        exit_error "configure_toe_server(): local test msg missing"
}


# Remote message / record checking

check_client_connected() {
    local rc=0
    local msg="addr=$LBLNET_SVR_IPV4 port=60 res=success"
    /sbin/ausearch -n "REMOTE_LOGGING_SERVER" -m "DAEMON_ACCEPT" \
        -ts $test_time_start | grep "$msg" || { rc=1 ;
        echo "Missing DAEMON_ACCEPT record" ; }

    return $rc
}

check_client_disconnected() {
    local rc=0
    local msg="addr=$LBLNET_SVR_IPV4 port=60 res=success"
    /sbin/ausearch -n "REMOTE_LOGGING_SERVER" -m "DAEMON_CLOSE" \
        -ts $test_time_start | grep "$msg" || { rc=1 ;
        echo "Missing DAEMON_CLOSE record" ; }

    return $rc
}

check_message_arrived() {
    local rc=0
    /sbin/ausearch -n "REMOTE_LOGGING_CLIENT" -m "USER" \
        -ts $test_time_start | egrep "type=USER .*${1}" || { rc=1 ;
        echo "Missing USER record from remote client" ; }

    #if [ $rc == 1 ] ; then
    #    echo "FAILED ---- grep -e \"type=USER .*${1}\"" >> /tmp/seq.log
    #    /sbin/ausearch -n "REMOTE_LOGGING_CLIENT" -m "USER" -ts $test_time_start >> /tmp/seq.log
    #    echo "----" >> /tmp/seq.log
    #    echo >> /tmp/seq.log
    #fi

    return $rc
}

check_server_accepted() {
    search_syslog " audisp-remote: Connected to $LBLNET_SVR_IPV4"
}

check_server_stop_service() {
    local msg="audisp-remote: remote logging disconnected due to remote server"
    msg+=" is going down, will attempt reconnection"
    # Note: The $msg above will be there only if we have emote_ending_action
    # configured to reconnect in /etc/audisp/audisp-remote.conf
    search_syslog "$msg"
}

check_client_disk_full_stop() {
    local timeout=100
    local rc=1
    for t in `seq $timeout` ; do
      sleep 1
      search_syslog "remote logging stopping due to remote server's disk is full, disk full" && \
      search_syslog "audisp-remote is exiting on stop request" && break
    done
    [ $t -ne $timeout ] && rc=0
    return $rc
}

check_client_disk_error_stop() {
    local timeout=100
    local rc=1
    for t in `seq $timeout` ; do
      # Sometimes it might happen that connection to remote server is lost because
      # of disk error, this is very rare and after restarting audit the connection
      # is re-established. This is not a bug of audit daemon.
      if search_syslog "audisp-remote: Error connecting to $LBLNET_SVR_IPV4"; then
          sleep 10
          restart_service auditd || exit_error "restart_service auditd"
      fi
      sleep 1
      search_syslog "remote logging stopping due to remote server has a disk error, disk write error" && break
    done
    [ $t -ne $timeout ] && rc=0
    return $rc
}


#
# Test set functions
#

# TOE acting as an audit logging server and Network server acts as an audit
# client

# Setup a remote client and trigger an audit record fo receive it.
test_server_basic() {
    test_time_start="`date +'%x %T'`"

    # Try to restore remote client configuration in case we exit prematurely
    prepend_cleanup " call_remote_function restore_audit_backup"

    # confiure TOE (local) as a server
    configure_toe_server || exit_error "configure_toe_server"

    # actual testing using a remote system
    call_remote_function configure_netsrv_client
    sleep 2
    check_client_connected || exit_fail "check_client_connected"

    call_remote_function generate_audit_log_msg

    sleep 2
    check_message_arrived "$remote_client_test_string"  || \
        exit_fail "check_message_arrived"
    # cleanup
    call_remote_function restore_audit_backup
    sleep 2
    check_client_disconnected || exit_fail "check_client_disconnected"
}

# Receive an exact sequence of for example 500 messages and make sure all
# arrived as expected
test_server_msg_sequence() {
    local missing_count=0
    test_time_start="`date +'%x %T'`"

    # Try to restore remote client configuration in case we exit prematurely
    prepend_cleanup "call_remote_function restore_audit_backup"

    # confiure TOE (local) as a server
    configure_toe_server || exit_error "configure_toe_server"

    # actual testing using a remote system
    call_remote_function configure_netsrv_client
    sleep 2
    check_client_connected || exit_fail "check_client_connected"

    call_remote_function generate_audit_log_msg_sequence

    # wait for message transmission
    sleep 5

    # on non-x86_64 archs transmission may take more time
    [[ $(uname -i) != "x86_64" ]] && sleep 15

    for i in `seq 1 $max_audit_log_dump_seq` ; do
        #echo -n "[$i]"  >> /tmp/seq.l g #  Uncomment for easier debugging
        check_message_arrived  "$remote_client_test_string SEQ_NUM=$i:?" || \
        ((missing_count+=1))
    done
    [ $missing_count -gt 0 ] && exit_error \
        "Missing $missing_count msgs out of $max_audit_log_dump_seq expected"

    call_remote_function restore_audit_backup
    sleep 2
    check_client_disconnected || exit_fail "check_client_disconnected"
}

# TOE acting as an audit client, Network server is an audit server

# Basic configuration test - TOE acting as client connecting to an audit server
test_client_basic(){
    test_time_start="`date +'%x %T'`"

    # Try to restore remote server configuration in case we exit prematurely
    prepend_cleanup "call_remote_function restore_audit_backup"

    call_remote_function configure_netsrv_server
    sleep 2
    mark_syslog
    # confiure TOE (local) as a client
    configure_toe_client || exit_error "configure_toe_client"
    sleep 2
    check_server_accepted || exit_fail "check_server_accepted"

    seq_num=$RANDOM
    /sbin/auditctl -m "$remote_client_test_string ($seq_num)"

    wait_for_cmd "call_remote_function check_msg_from_client | \
        grep -e \"type=USER.*$remote_client_test_string ($seq_num)\"" || \
        exit_fail "check_msg_from_client"

    call_remote_function stop_remote_auditd
    sleep 2
    check_server_stop_service || exit_fail "check_server_stop_service"
}

test_client_disk_full() {
    test_time_start="`date +'%x %T'`"

    # Try to restore remote server configuration in case we exit prematurely
    prepend_cleanup "call_remote_function restore_audit_backup"

    # NS is acting as a remote audit logging server
    call_remote_function configure_netsrv_server_disk_full
    sleep 2
    mark_syslog # Enjoy faster lookups
    # confiure TOE (local) as a client for disk full test
    configure_toe_client_disk_action "full" "STOP" || \
        exit_error "configure_toe_client_disk_full"
    sleep 2
    check_server_accepted || exit_fail "check_server_accepted"

    seq_num=$RANDOM
    /sbin/auditctl -m "$remote_client_test_string ($seq_num)"

    wait_for_cmd "call_remote_function check_msg_from_client | \
        grep -e \"type=USER.*$remote_client_test_string ($seq_num)\"" || \
        exit_fail "check_msg_from_client"

    # We are good to start testing the disk_full action (default is stop)
    call_remote_function ns_simulate_disk_full
    # Trigger action of the remote plugin so we can check for configured action.
    # Server should report about full disk back to the plugin.
    auditctl -m "$remote_client_test_string"
    check_client_disk_full_stop || exit_fail "check_client_disk_full_stop"
}

test_client_disk_error() {
    test_time_start="`date +'%x %T'`"

    # Try to restore remote server configuration in case we exit prematurely
    prepend_cleanup "call_remote_function restore_audit_backup"

    # NS is acting as a remote audit logging server
    call_remote_function configure_netsrv_server_disk_error
    sleep 2
    mark_syslog # Enjoy faster lookups
    # confiure TOE (local) as a client for disk full test
    configure_toe_client_disk_action "error" "STOP" || \
        exit_error "configure_toe_client_disk_full"
    sleep 2
    check_server_accepted || exit_fail "check_server_accepted"

    seq_num=$RANDOM
    /sbin/auditctl -m "$remote_client_test_string ($seq_num)"

    wait_for_cmd "call_remote_function check_msg_from_client | \
        grep -e \"type=USER.*$remote_client_test_string ($seq_num)\"" || \
        exit_fail "check_msg_from_client"

    # We are good to start testing the disk_full action (default is stop)
    call_remote_function ns_simulate_disk_error
    # Trigger action of the remote plugin so we can check for configured action.
    # Server should report error disk back to the plugin.
    auditctl -m "$remote_client_test_string"
    check_client_disk_error_stop || exit_fail "check_client_disk_error_stop"
}


#
# Run specified test
#

if [ "$(type -t test_$scenario)a" == "functiona" ] ; then
    test_$scenario
else
    exit_error "There is no such test scenario ($scenario)"
fi

# Looks like we are good to pass!
exit_pass
