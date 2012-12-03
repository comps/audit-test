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
# Entry point for helper functions to be triggred by lblnet_tst_server.
#
# AUTHOR: Eduard Benes <ebenes@redhat.com>
#
# DESCRIPTION:
#  We want to use lblnet_tst_server to use this script as an entry point when
#  accessing other functions required for testing audit remote loging on a two
#  physically separate systems.
#  Both server and client will use TCP port 60 for remote connections.
#


# if called by xinetd, we will have trouble getting to our function files
export TOPDIR=/usr/local/eal4_testing/audit-test/
export PATH=$PATH:$TOPDIR
export PATH=$PATH:$TOPDIR/audit-remote/tests/
export PATH=$PATH:$TOPDIR/utils/

source audisp-remote_functions.bash || exit 2

action="$1"
mode="$2"
caller_ipv4="$3"

# List of  audit files we need to backup before a testing
audit_files="/etc/audit/auditd.conf /etc/audisp/audisp-remote.conf
  /etc/audisp/audispd.conf /etc/audisp/plugins.d/au-remote.conf"
conf_backup="/tmp/audit_conf_backup.tgz"


#
# General helper functions
#

# Configuration related functions

remote_audit_backup() {
    tar -cvz --xattrs -f $conf_backup $audit_files
}

# Configure NS to act as a server
write_server_configs() {
    # Make sure we are not acting also as a client
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
}

# Configure NS to act as a client
write_client_configs() {
    # Netserver is a client and TOE is acting as a server in this case
    write_config -r "$auditd_conf" tcp_listen_port
    write_config -s "$auditd_conf" \
        name_format=user \
        name=REMOTE_LOGGING_CLIENT \
        dispatcher=/sbin/audispd \
        disp_qos=lossless

    write_config -s "$audispd_conf" \
        name_format=user \
        name=REMOTE_LOGGING_CLIENT
        # q_depth = 200 # We might want to have bigger queue for some tests

    # Netserver is a client and TOE is acting as a server in this case
    write_config -s "$audisp_remote_conf" \
	mode=$mode \
        remote_ending_action=reconnect \
        remote_server=$caller_ipv4
        # queue_depth = 4096 # Possible to increase queue for mode=forward

    write_config -s "$au_remote_conf" \
        active=yes
}


#
# Remotely "call-able" functions
#

# Debugging call, it is not used in the test suite at all
call_ns_connection_check() {
    echo "NS call values: action=$action, mode=$mode, caller_ipv4=$caller_ipv4"
    logger "call_ns_connection(): success on NS side"
}

# Configuration of a remote system

call_configure_netsrv_client() {
    mark_syslog
    # TOE will act as a server and IP addr is in LOCAL_IPV4 from net profile
    ns_service_auditd_stop || \
        logger "ERROR: ns_service_auditd_stop(): auditd is still runnning"
    # Save current configuration before any changes
    remote_audit_backup

    # Configure to act as a client
    # Make sure server is there waiting for us
    /bin/ping -c 3 $caller_ipv4 || \
        logger "ERROR call_configure_netsrv_client(): server not reachable"

    # put our configs in place
    write_client_configs

    ns_service_auditd_start || \
        logger "ERROR: ns_service_auditd_start(): auditd failed to start"
    # Make sure Network Server connected to the TOE as audit remote client
    search_syslog " audisp-remote: Connected to $caller_ipv4" || \
        logger "Client (NS) failed to connect to TOE acting as a server"
}

call_configure_netsrv_server() {
    local test_time_start="`date +'%x %T'`"
    local msg="`create_user_test_msg`"

    # Netserver is a client and TOE is acting as a server in this case
    ns_service_auditd_stop || \
        logger "ERROR: ns_service_auditd_stop(): auditd is still runnning"
    # Save current configuration before any changes
    remote_audit_backup

    # Make sure server is there waiting for us
    /bin/ping -c 3 $caller_ipv4 || \
        logger "ERROR all_configure_netsrv_server(): client not reachable"

    # Configure to act as a server
    # Put configs in place
    write_server_configs

    ns_service_auditd_start || \
        logger "ERROR: ns_service_auditd_start(): auditd failed to start"
    sleep 2
    # Are we ready to start testing?
    auditctl -m "$msg"
    sleep 1
    ausearch -m USER -ts $test_time_start | grep "$msg" || logger \
        "ERROR call_configure_netsrv_server(): could not find local test message"
}

# Backup all relevant audit files

call_restore_audit_backup() {
set -x
    restorecon -RFv /var/log/audit
    ns_service_auditd_stop || \
        logger "ERROR: ns_service_auditd_stop(): auditd is still runnning"
    sleep 2
    /bin/tar -C / --overwrite -xvzf  $conf_backup
    # We want to make sure the default system configration is restored
    # in case tmpfs is still mounted after testing or if we've lost mount
    umount -lf /var/log/audit
    mount -o remount /var/log/audit || mount /var/log/audit
    restorecon -RFv /var/log/audit
    chmod 750 /var/log/audit
    ns_service_auditd_start || \
        logger "ERROR: ns_service_auditd_start(): auditd failed to start"
set +x
    # Dump configs for better debugging on server side
    for f in $audit_files ; do echo -e "\n---- $f ----\n" ; cat $f ; done
}

call_stop_remote_auditd() {
    ns_service_auditd_stop
}

call_show_audit_files() {
    for f in $audit_files ; do
        echo -e "\n---- $f ----\n"
        cat $f
    done
}

# Message generation functions

call_generate_audit_log_msg() {
    # This generates following AVC messages, but seems to work as we need it to
    #============= auditctl_t ==============
    #allow auditctl_t inetd_t:tcp_socket { read write };
    # There is a fix available in lblnet_tst_server.c, but disabled by default.
    # See ctl_audit_remote_call() and uncomment net_hlp_socket_close(&sock).
    /sbin/auditctl -m "$remote_client_test_string ($RANDOM)"
}

call_generate_audit_log_msg_sequence() {
    # generate a lot of messages in audit log, e.g. 500 so we can check for them
    # later on an audit server whether they all arrived
    sleep 2
    /sbin/auditctl -s # check how many lost messages we have before we start
    for i in `seq 1 $max_audit_log_dump_seq` ; do
        /sbin/auditctl -m "$remote_client_test_string SEQ_NUM=$i"
        # Give some time to catch up to the logging chain:
        #   auditd -> audispd -> (logging proces + audisp-remote)
        ((m=$i%25))
        if [ $m == 0 ] ; then
            sleep 1
	    /sbin/auditctl -s
        fi
    done
    sleep 3 # Give it some time appear in audit.log
}

call_check_msg_from_client() {
    # Using -ts recent for the sake of simplicity
    /sbin/ausearch -n "REMOTE_LOGGING_CLIENT" -m "USER" \
        -ts recent | grep -e "type=USER .*${remote_client_test_string}" || \
        logger "Missing USER record from a remote client with address $caller_ipv4"
}


# Functions related to an action upon reaching configured treshold

call_ns_simulate_disk_error() {
    local test_msg=${1:-"`create_user_test_msg`"}

    # Simulate disk error
    chcon system_u:object_r:games_data_t:s0 /var/log/audit/audit.log
    write_local_records 1 || logger "write_local_records failed"
}

call_configure_netsrv_server_disk_error() {
    local test_time_start="`date +'%x %T'`"
    local msg="`create_user_test_msg`"

    # Netserver is a client and TOE is acting as a server in this case
    ns_service_auditd_stop || \
        logger "ERROR: ns_service_auditd_stop(): auditd is still runnning"
    # Save current configuration before any changes
    remote_audit_backup

    # Make sure server is there waiting for us
    /bin/ping -c 3 $caller_ipv4 || \
        logger "ERROR all_configure_netsrv_server(): client not reachable"

    # Configure NS to act as a server for disk_full testing
    # Make sure we are not acting also as a client
    write_config -s "$au_remote_conf" \
        active=no

    write_config -s "$auditd_conf" \
        log_file=/var/log/audit/audit.log \
        log_format=RAW \
        num_logs=5 \
        max_log_file_action=IGNORE \
        space_left=2 \
        space_left_action=IGNORE \
        admin_space_left=1 \
        admin_space_left_action=IGNORE \
        disk_full_action=IGNORE \
        disk_error_action=IGNORE \
        action_mail_acct=root \
        tcp_listen_port=60 \
        name_format=user \
        name=REMOTE_LOGGING_SERVER \
        flush=INCREMENTAL \
        freq=20
#        flush=SYNC

    write_config -s "$audispd_conf" \
        name_format=user \
        name=REMOTE_LOGGING_SERVER

#    # use 8MB tmpfs for audit logs for easier disk_full simulation
#    mount -t tmpfs -o size=$((1024 * 1024 * 8)) none /var/log/audit
#    chmod 750 /var/log/audit
#    selinuxenabled && chcon system_u:object_r:auditd_log_t:s0 /var/log/audit

    ns_service_auditd_start || \
        logger "ERROR: ns_service_auditd_start(): auditd failed to start"
    sleep 2
    # Are we ready to start testing?
    auditctl -m "$msg"
    sleep 1
    ausearch -m USER -ts $test_time_start | grep "$msg" || logger \
        "ERROR call_configure_netsrv_server_disk_error(): could not find local test message"
}

call_configure_netsrv_server_disk_full() {
    local test_time_start="`date +'%x %T'`"
    local msg="`create_user_test_msg`"

    # Netserver is a client and TOE is acting as a server in this case
    ns_service_auditd_stop || \
        logger "ERROR: ns_service_auditd_stop(): auditd is still runnning"
    # Save current configuration before any changes
    remote_audit_backup

    # Make sure server is there waiting for us
    /bin/ping -c 3 $caller_ipv4 || \
        logger "ERROR all_configure_netsrv_server(): client not reachable"

    # Configure NS to act as a server for disk_full testing
    # Make sure we are not acting also as a client
    write_config -s "$au_remote_conf" \
        active=no

    write_config -s "$auditd_conf" \
        log_file=/var/log/audit/audit.log \
        log_format=RAW \
        num_logs=5 \
        max_log_file_action=IGNORE \
        space_left=2 \
        space_left_action=IGNORE \
        admin_space_left=1 \
        admin_space_left_action=IGNORE \
        disk_full_action=IGNORE \
        disk_error_action=IGNORE \
        action_mail_acct=root \
        tcp_listen_port=60 \
        name_format=user \
        name=REMOTE_LOGGING_SERVER \
        flush=SYNC
#        flush=INCREMENTAL
#        freq=20

    write_config -s "$audispd_conf" \
        name_format=user \
        name=REMOTE_LOGGING_SERVER

    # use 8MB tmpfs for audit logs for easier disk_full simulation
    mount -t tmpfs -o size=$((1024 * 1024 * 8)) none /var/log/audit
    chmod 750 /var/log/audit
    selinuxenabled && chcon system_u:object_r:auditd_log_t:s0 /var/log/audit

    ns_service_auditd_start || \
        logger "ERROR: ns_service_auditd_start(): auditd failed to start"
    sleep 2
    # Are we ready to start testing?
    auditctl -m "$msg"
    sleep 1
    ausearch -m USER -ts $test_time_start | grep "$msg" || logger \
        "ERROR call_configure_netsrv_server_disk_full(): could not find local test message"
}

call_ns_simulate_disk_full() {
    local test_msg=${1:-"`create_user_test_msg`"}

    # Fill the local filesystem hosting audit.log, leaving 5k available
    # Compensate for 64k page size on power
    if [[ $ARCH != "PPC" ]]; then
       fill_disk ${audit_log%/*} 5 || logger "fill_disk failed"
    else
       fill_disk ${audit_log%/*} 70 || logger "fill_disk failed"
    fi

    # each record is at least 80 bytes (based on empirical evidence), so writing
    # 65 records should always take us over (65 * 80 =~ 5k)
    write_local_records 65 || logger "write_local_records failed"
}


#
# The most interesting part for us
#

if [ "$(type -t call_${action})a" = "functiona" ] ; then
    # call_ functions are going to be running as a single calls locally issued
    # by lblnet_tst_server upon receiving a connection
    logger "Received request to execute function call_${action}()"
    logger "Rotating audit.log"
    # We don't want to send the whole log to a caller, start with rotated log
    ns_service_auditd_rotate
    sleep 2
    call_$action
    sleep 2
    logger "Executed function call_${action}()"
else
    logger "Failed to execute action ${action}"
    exit 2
fi


