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
# Collection of helper functions used by audit remote logging tests.
#
# AUTHOR: Eduard Benes <ebenes@redhat.com>
#
# DESCRIPTION:
#   Test sourcing this file expects one or two arguments:
#      testname <action> [mode]
#
#   Where action parameter is one of halt, single, suspend, stop, syslog, exec,
#   ignore, email. And mode parameter can be immediate or forward. If no mode
#   parameter is provided, immediate will be used by default for audisp-remote.
#


source testcase.bash || exit 2

#
# Global variables
#

debug=""
test_suite_path="/usr/local/eal4_testing/audit-test"
# get recipient of root mail from /etc/aliases "root: jdoe, jsmith" line
ralias=$(sed -n 's/^root:[ \t]*\([^,]*\).*/\1/p' /etc/aliases)
[ -z $ralias ] && ralias=root
eal_mail="/var/mail/$ralias"
unset ralias

syslog_file="/var/log/messages"
syslog_mark=0

action=$1
mode=$2

total_written=0

# Variables used by basic connection for TOE acting as server and client

local_audit_server_ip=`hostname --ip-address`
local_audit_server_hostname=`hostname`
ping $local_audit_server_ip -c 1 || exit_error "Unable to ping audit server"
ping $local_audit_server_hostname -c 1 || exit_error "Unable to ping audit server"

auditd_conf="/etc/audit/auditd.conf"
audisp_remote_conf="/etc/audisp/audisp-remote.conf"
au_remote_conf="/etc/audisp/plugins.d/au-remote.conf"
audispd_conf="/etc/audisp/audispd.conf"

audit_log="/var/log/audit/audit.log"
# Default strategy for handling records in audisp-remote.conf in none given
remote_loggig_mode=${mode:-"immediate"}

# Drop a temporary file to be used for testing triggering exec actions.
if [ "$action" == "exec" ] ; then
    exec_file="`mktemp ${test_suite_path}/audit-remote/exec_action_test.sh.XXX`"
    chmod 0750 $exec_file # This is required by the audisp-remote plugin
    append_cleanup "rm -f $exec_file"
fi

# Max seq number / upper boundary expected from a remote logging client
max_audit_log_dump_seq=100

# User message we will be expected by checking functions in audit.log
remote_client_test_string="Client record for audit remote logging test"

#
# Functions
#

# Auditd service handling

ns_service_auditd_stop() {
    expect -c "
    spawn /usr/sbin/run_init /sbin/service auditd stop
    expect {
        -nocase \"password: \" {send \"$PASSWD\\r\"; exp_continue}
        eof
    }"
    /sbin/service auditd status && rc=1
    return ${rc:-0}
}

ns_service_auditd_start() {
    expect -c "
    spawn /usr/sbin/run_init /sbin/service auditd start
    expect {
        -nocase \"password: \" {send \"$PASSWD\\r\"; exp_continue}
        eof
    }"
    /sbin/service auditd status
    return $?
}

ns_service_auditd_restart() {
    expect -c "
    spawn /usr/sbin/run_init /sbin/service auditd restart
    expect {
        -nocase \"password: \" {send \"$PASSWD\\r\"; exp_continue}
        eof
    }"
    /sbin/service auditd status
    return $?
}

ns_service_auditd_rotate() {
    expect -c "
    spawn /usr/sbin/run_init /sbin/service auditd rotate
    expect {
        -nocase \"password: \" {send \"$PASSWD\\r\"; exp_continue}
        eof
    }"
    /sbin/service auditd status
    return $?
}


# System configuration functions.
# NOTE: Make sure there is a backup of all relevant audit config files
# before calling this functions.

# remote-server-ipv4, mode
configure_audit_client() {
    audit_server_ipv4=${1:-$LBLNET_SVR_IPV4}
    write_config -s $audisp_remote_conf remote_server=$audit_server_ipv4
    write_config -s $audisp_remote_conf local_port=60
    write_config -s $audisp_remote_conf mode=${2:-"immediate"}
}

configure_audit_server() {
    stop_auditd

    # Update the config to accept connections
    write_config -s $auditd_conf tcp_listen_port=60
    write_config -s $auditd_conf tcp_client_ports=61

    if [ "${1}a" == "audispda" ] ; then
        write_config -s $auditd_conf "dispatcher=/sbin/audispd"
    else
        # Do not use dispatcher for auditd at all
        write_config -r $auditd_conf "dispatcher"
    fi

    start_auditd || exit_error "Failed to start auditd service"
}


# Functions for basic connection for TOE acting as server and client
# See test_audisp-remote_connection.bash for usage example.
# NOTE: Make sure there is a backup of all relevant audit config files
# before calling this function.

configure_local_audit_server() {
    stop_auditd

    # Update the config to accept connections
    write_config -s $auditd_conf tcp_listen_port=60
    write_config -s $auditd_conf tcp_client_ports=61

    if [ "${1}a" == "audispda" ] ; then
        write_config -s $auditd_conf "dispatcher=/sbin/audispd"
    else
        # Do not use dispatcher for auditd at all
        write_config -r $auditd_conf "dispatcher"
    fi

    start_auditd || exit_error "Failed to start auditd service"
}

configure_local_audisp_remote() {
    write_config -s $audisp_remote_conf remote_server=$local_audit_server_hostname
    write_config -s $audisp_remote_conf local_port=61
    write_config -s $audisp_remote_conf mode=$remote_loggig_mode
}


# Message handling (mostly for local tests when TOE acts as server and client)

create_user_test_msg() {
    /sbin/auditctl -m "audisp-remote test message"
    msg="`tail $audit_log | grep 'type=USER.*audisp-remote test message'`"
    [ -z "$msg" ] && exit_error "Failed to create USER test message"
    echo "$msg"
}

send_audisp_remote_test_msg() {
    [ -z "$*" ] && exit_error \
        "send_audisp_remote_test_msg: no message to send"
    echo "$*" | /sbin/audisp-remote
}

check_received_test_msg() {
    # Accept custom log files for augrok to search
    [ ! -z $1 ] && opt="--file=$1"

    # Latest audit version correctly removes port from addr field, uncomment
    # for old version.
    #local client_addr_port="${local_audit_server_ip}:61"
    #msg_accept="type=DAEMON_ACCEPT msg=audit(.*): addr=$client_addr_port port=61 res=success"
    msg_accept="type=DAEMON_ACCEPT msg=audit(.*): addr=$local_audit_server_ip port=61 res=success"
    msg_test="type=USER.*audisp-remote test message"
    #msg_close="type=DAEMON_CLOSE msg=audit(.*): addr=$client_addr_port port=61 res=success"
    msg_close="type=DAEMON_CLOSE msg=audit(.*): addr=$local_audit_server_ip port=61 res=success"

    # Check message with augrok
    augrok $opt "type=DAEMON_ACCEPT" | grep "$msg_accept" || \
        exit_fail "Missing DAEMON_ACCEPT record"
    augrok $opt "type=USER"          | grep "$msg_test" || \
        exit_fail "Missing USER test record"
    augrok $opt "type=DAEMON_CLOSE"  | grep "$msg_close" || \
        exit_fail "Missing DAEMON_CLOSE record"
}


# Disk full action functions

configure_local_auditd_disk_full_action() {
    auditd_action=${1:-$action}
    write_config -s $auditd_conf \
        admin_space_left=1 \
        admin_space_left_action=IGNORE \
        disk_error_action=IGNORE \
        disk_full_action=$auditd_action \
        space_left=2
}

configure_local_plugin_disk_full_action() {
    remote_action=${1:-$action}
    write_config -s $audisp_remote_conf \
        disk_full_action=$remote_action \
        disk_error_action=IGNORE \
        disk_low_action=IGNORE
    if [ $remote_action == "exec" ] ; then
        # Handle special case for exec action executing our exec_file
        write_config -s $audisp_remote_conf \
            "disk_full_action=exec $exec_file"
    fi
}

# Disk low action functions

configure_local_auditd_disk_low_action() {
    auditd_action=${1:-$action}
    write_config -s $auditd_conf \
        admin_space_left=1 \
        disk_full_action=IGNORE \
        disk_error_action=IGNORE \
        admin_space_left_action=$auditd_action \
        space_left=2
}

configure_local_plugin_disk_low_action() {
    remote_action=${1:-$action}
    if [ $remote_action == "exec" ] ; then
        # Handle special case for exec action executing our exec_file
        write_config -s $audisp_remote_conf \
            "disk_low_action=exec $exec_file"
    else
        write_config -s $audisp_remote_conf disk_low_action=$remote_action
    fi
}

# Disk error action functions

configure_local_auditd_disk_error_action() {
    auditd_action=${1:-$action}
    write_config -s $auditd_conf \
        admin_space_left=1 \
        disk_error_action=$auditd_action \
        space_left=2
}

configure_local_plugin_disk_error_action() {
    remote_action=${1:-$action}
    write_config -s $audisp_remote_conf \
        disk_error_action=$remote_action

    if [ $remote_action == "exec" ] ; then
        # Handle special case for exec action executing our exec_file
        write_config -s $audisp_remote_conf \
            "disk_error_action=exec $exec_file"
    fi
}

# Remote ending action functions

configure_local_auditd_remote_ending_action() {
    write_config -s $auditd_conf \
        admin_space_left=1 \
        space_left=2
}

configure_local_plugin_remote_ending_action() {
    remote_action=${1:-$action}
    write_config -s $audisp_remote_conf \
        remote_ending_action=$remote_action

    if [ $remote_action == "exec" ] ; then
        # Handle special case for exec action executing our exec_file
        write_config -s $audisp_remote_conf \
            "remote_ending_action=exec $exec_file"
    fi
}

# Network failure action functions

configure_local_auditd_network_failure_action() {
    configure_local_auditd_remote_ending_action
}

configure_local_plugin_network_failure_action() {
    remote_action=${1:-$action}
    write_config -s $audisp_remote_conf \
        network_failure_action=$remote_action

    if [ $remote_action == "exec" ] ; then
        # Handle special case for exec action executing our exec_file
        write_config -s $audisp_remote_conf \
            "network_failure_action=exec $exec_file"
    fi
}


# Disk action testing helper functions

# write_local_records(num): writes num records to the audit log
write_local_records() {
    declare max=$1
    echo "Writing records to local audit log ($max)"
    for ((i=1; i<=max; i++)); do
        auditctl -m "$action $i" || return 2
        auditctl -r 0 >/dev/null || return 2
        if ((i % 10 || i+1 == max)); then
            echo -n .
        else
            echo -n $i
        fi
    done
    echo
    (( total_written += max ))
    echo "Total written = $total_written"
    ls -lk ${audit_log}*
    df -k ${audit_log%/*}/
}

# fill_disk(dir, free): creates a file in dir which fills the filesystem,
# leaving free kb available
fill_disk() {
    # So far, this only happens on tmpfs, so don't worry about deleting the temp
    # file later.
    declare dir=$1 free=$2
    declare bloat=$dir/bloat
    declare before during after
    declare header=$(df -k "$dir" | head -n1)
    echo
    echo "Filling $dir, leaving $free KB free"
    before=$(df -k "$dir" | sed -rn '2s/^.{7}/BEFORE /p')

    cat </dev/zero >"$bloat" 2>/dev/null
    during=$(df -k "$dir" | sed -rn '2s/^.{7}/DURING /p')

    free=$free bloat=$bloat perl -we '
        die "failed to truncate $ENV{bloat}\n" unless
        truncate $ENV{"bloat"}, (stat $ENV{"bloat"})[7] - 1024*$ENV{"free"}' \
        || return 2
    after=$(df -k "$dir" | sed -rn '2s/^.{7}/AFTER  /p')

    printf "%s\n%s\n%s\n%s\n\n" "$header" "$before" "$during" "$after"
}


# Syslog helper functions

# mark_syslog(msg): marks syslog, with an optional msg for later searching
mark_syslog() {
    syslog_mark=$(wc -l <$syslog_file)
    #logger "auditd_testing: $$ $msg"
    logger "audisp-remote_testing: $$ $msg"
}

# search_syslog(msg): searches syslog, starting at $syslog_mark, looking for
# "msg"
search_syslog() {
    declare syslog syslog_filtered syslog_output
    sleep 0.1   # let syslog catch up

    # Shift through the syslog messages
    syslog=$(sed "1,${syslog_mark}d" "$syslog_file")
    syslog_filtered=$(grep -F "$1" <<<"$syslog")
    if [[ -n $syslog_filtered ]] ; then
        syslog_output="$syslog_filtered"
    else
        syslog_output="$syslog"
    fi

    # Output a syslog snapshot for a possible log review
    echo
    echo "Syslog snippet"
    echo "--- start messages ---------------------------------------------------------"
    echo "$syslog_output"
    echo "--- end messages -----------------------------------------------------------"

    # Return the correct error code and output error messages if required
    if [[ -n $syslog_filtered ]] ; then
        return 0
    elif grep -Fq " auditd[$(pidof auditd)]: " <<<"$syslog"; then
        echo "search_syslog: couldn't find auditd message in syslog"
        return 1
    else
        echo "search_syslog: either auditd or syslog is broken"
        return 2
    fi
}

# Remove syslog messages after last call of mark_syslog function
restore_syslog() {
    local tmp_syslog=`mktemp`
    cp $syslog_file $tmp_syslog
    sed -n "1,${syslog_mark}p" $tmp_syslog > $syslog_file
    rm -f $tmp_syslog
    # This could be removed, but we want to be sure log file is sane.
    restorecon -RvvF $syslog_file
}

pre_keep_logs() {
    # should be 2, but just to be certain... and make this global
    num_logs=$(awk '/^num_logs =/{print $NF;exit}' "$auditd_conf")
    declare i
    for ((i = 1; i < num_logs * 2; i++)); do
        echo "type=AGRIFFIS" >> "$audit_log.$i"
    done
}

check_keep_logs() {
    declare i
    for ((i = 2; i < num_logs * 2 + 1; i++)); do
        if ! grep -Fqx "type=AGRIFFIS" "$audit_log.$i"; then
            echo "check_keep_logs: failed checking $audit_log.$i"
            return 1
        fi
    done
    return 0
}


# Functions for checking behaviour of audisp-remote plugin on actions

# Ignore action functions

# Nothing to test here, allright?
remote_check_ignore() {
    echo "$0 not implemented"
    echo_error
}

# Syslog action functions

pre_syslog() {
    mark_syslog
}

remote_check_syslog() {
    search_syslog " audisp-remote: $@"
}

# Suspend action functions

pre_suspend() {
    mark_syslog
}

remote_check_suspend() {
    search_syslog " audisp-remote: $*"
}

# Stop action functions

pre_stop() {
    mark_syslog
}

remote_check_stop() {
    search_syslog " audisp-remote: $*"
}

# Exec action helper functions

pre_exec() {
    append_cleanup "rm -rf $exec_file"
    # Prepare testing executable file
    cat > $exec_file << EOT
#!/bin/bash
/usr/bin/logger "Audisp-remote exec action testing."
EOT
    chmod +x $exec_file
    mark_syslog
    # Make sure we can check if we fail to create exec_file
    [ -f $exec_file ]
}

remote_check_exec() {
    ls -Z $exec_file
    cat $exec_file
    search_syslog "Audisp-remote exec action testing."
}

# Halt and single action functions

pre_halt() {
    declare mask_runlevel=${1:-0}

    mark_syslog

    # replace /sbin/init with our own version
    append_cleanup '
    if [[ -s /sbin/init.audisp-remote_testing ]]; then
        mv /sbin/init.audisp-remote_testing /sbin/init
        restorecon /sbin/init
    fi'
    if [[ ! -s /sbin/init.audisp-remote_testing ]]; then
        mv /sbin/init /sbin/init.audisp-remote_testing
    fi
    cat >/sbin/init <<EOF
#!/bin/bash -x
if [[ \$1 == $mask_runlevel ]]; then
    logger "audisp-remote_testing: $$ runlevel=\$1"
else
    exec /sbin/init.audisp-remote_testing "\$@"
fi
EOF

    # make sure the new version has the right security attributes
    restorecon -Fv /sbin/init
    chmod +x /sbin/init
    # Make sure the fake /sbin/init is younger or we've most likely failed
    # to created it.
    [ "/sbin/init.audisp-remote_testing" -ot "/sbin/init" ]
}

remote_check_halt() {
    search_syslog " audisp-remote_testing: $$ runlevel=${1:-0}"
}

# Single action functions

pre_single() {
    pre_halt 1
}

remote_check_single() {
    remote_check_halt 1
}

# Reconnect action functions
# Note: This is specific to remote_ending_action option only.

pre_reconnect() {
    mark_syslog
}

remote_check_reconnect() {
    search_syslog " audisp-remote: $*"
}

# Email action functions
# Note: This action is currently not supported in audispd-plugins
# version <= 2.1-5.el6, therefore there are no tests for this yet.

pre_email() {
    if [ -f $eal_mail ] ; then
        eal_mail_lines=$(wc -l < "$eal_mail")
    else
        eal_mail_lines=0
    fi
}

remote_check_email() {
    sleep 2     # wait for the mail to arrive

    if augrok -q type=AVC comm=auditd 'name=~sendmail'; then
        exit_error "AVC blocked auditd running sendmail"
    fi

    if [ ! -f $eal_mail ] ; then
        echo "remote_check_email: "$eal_mail" does not exist"
        return 2
    fi
    # Look for new mail
    [ -z $debug ] || sed "1,${eal_mail_lines}d" "$eal_mail"
    if sed "1,${eal_mail_lines}d" "$eal_mail" | grep -Fq "$1"; then
        echo "remote_check_email: found magic email"
        return 0
    else
        echo "remote_check_email: email never arrived"
        return 1
    fi
}


# Function to prepare basic server pre-configuration

common_server_startup() {
    local rc=0

    # use 8MB tmpfs for audit logs
    mount -t tmpfs -o size=$((1024 * 1024 * 8)) none /var/log/audit || ((rc+=1))

    chmod 750 /var/log/audit || ((rc+=2))
    selinuxenabled && chcon system_u:object_r:auditd_log_t:s0 /var/log/audit

    prepend_cleanup '
        umount -l /var/log/audit
        restart_auditd'

    backup $auditd_conf
    backup $audisp_remote_conf

    # Explicit custom-default config to prevent unexpected issues
    write_config -s "$auditd_conf" \
        log_file=/var/log/audit/audit.log \
        log_format=RAW \
        num_logs=5 \
        max_log_file=1024 \
        max_log_file_action=IGNORE \
        space_left=2 \
        space_left_action=IGNORE \
        admin_space_left=1 \
        admin_space_left_action=IGNORE \
        disk_full_action=IGNORE \
        disk_error_action=IGNORE \
        action_mail_acct=root \
        flush=INCREMENTAL \
        freq=20

    write_config -s "$audisp_remote_conf" \
        network_failure_action=IGNORE \
        disk_error_action=IGNORE \
        remote_ending_action=IGNORE \
        generic_error_action=IGNORE \
        generic_warning_action=IGNORE \
        disk_full_action=IGNORE \
        disk_low_action=IGNORE

    if [ "$(type -t pre_$action)a" = "functiona" ] ; then
        pre_$action || ((rc+=4))
    fi
    return $rc
}

print_audit_log_snippet() {
    echo
    echo "Audit log snippet"
    echo "--- start audit.log --------------------------------------------------------"
    augrok type!=AGRIFFIS not \( type=CONFIG_CHANGE audit_rate_limit=0 \)
    echo "--- end audit.log ----------------------------------------------------------"
}


#
# common startup
#

# Leaving this in a commment to surpess the verbosity by default.
#prepend_cleanup print_audit_log_snippet

if [ -z "$action" ] ; then
    echo "Please specify an action on the command-line"
    exit 2
fi
