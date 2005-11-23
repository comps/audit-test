#!/bin/bash -e
# =============================================================================
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
# Written by Aron Griffis <aron@hp.com>
#
#   This program is free software;  you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY;  without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
#   the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program;  if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
# =============================================================================

######################################################################
# global vars
######################################################################

auditd_conf=/etc/auditd.conf
auditd_orig=$(mktemp $auditd_conf.XXXXXX)
audit_log=/var/log/audit/audit.log
eal_mail=/var/mail/eal
messages=/var/log/messages
tmp1=$(mktemp)
tmp2=$(mktemp)
zero=${0##*/}

######################################################################
# common functions
######################################################################

function write_auditd_conf {
    declare x key value

    # Save off the auditd configuration
    if [[ ! -s "$auditd_orig" ]]; then 
	cp -a "$auditd_conf" "$auditd_orig"
    fi

    # Replace configuration lines; slow but works
    for x in "$@"; do
	key=${x%%=*}
	value=${x#*=}
	sed -i "/^$key[[:blank:]]*=/d" "$auditd_conf"
	echo "$key = $value" >> "$auditd_conf"
    done

    cat "$auditd_conf"
}

function write_records {
    echo "Writing records to audit log ($ctr)"
    while true; do
	(( ctr++ ))
	auditctl -m "$zero $ctr"
	if (( ctr % $1 == 0 )); then break; fi
    done
}

function cleanup {
    auditctl -D
    service auditd stop ||:
    umount /var/log/audit ||:
    if [[ -s "$auditd_orig" ]]; then mv "$auditd_orig" "$auditd_conf"; fi
    service auditd start ||:
    rm -f "$auditd_orig" "$tmp1" "$tmp2"
}

function check_rotate {
    if [[ ! -s "$audit_log.1" ]]; then
	echo "check_rotate: log did not rotate"
	return 1
    fi

    # verify the logs were written correctly (no missing or additional records)
    grep -aho "$zero [[:digit:]]*" "$audit_log.1" "$audit_log" >"$tmp1"
    seq 1 $ctr | sed "s/^/$zero /" >"$tmp2"
    cmp "$tmp1" "$tmp2" || return 1	# could be "diff" for debugging
}

function pre_syslog {
    logger "making sure syslog works $$"
}

function check_syslog {
    declare pid=$(service auditd status | grep -Eo '[0-9]+')
    if grep -Fq " auditd[$pid]: Audit daemon log file is larger than max size" "$messages"; then
	return 0
    elif grep -Fq "making sure syslog works $$" "$messages"; then
	echo "check_syslog: syslog appears to be broken"
	return 2
    else
	echo "check_syslog: couldn't find auditd syslog record"
	return 1
    fi
}

function check_ignore {
    declare size1=$(stat -c %s /var/log/audit/audit.log) size2
    write_records 10
    sleep 0.1
    size2=$(stat -c %s /var/log/audit/audit.log)
    if (( size2 < size1 )); then
	echo "check_ignore: audit.log appears to have shrunk!"
	return 2
    elif (( size2 == size1 )); then
	echo "check_ignore: audit.log is not growing"
	return 1
    else
	return 0
    fi
}

function check_suspend {
    declare status=0
    check_ignore || status=$?
    case $status in
	0) echo "check_suspend: audit.log is still growing, this is bad"; return 1 ;;
	1) echo "check_suspend: audit.log is suspended, this is good"; return 0 ;;
	*) echo "check_suspend: check_ignore returned error status"; return $status ;;
    esac
}

function pre_keep_logs {
    # should be 2, but just to be certain... and make this global
    num_logs=$(awk '/^num_logs =/{print $NF;exit}' "$audit_log")
    declare i
    for ((i = 1; i < num_logs * 2; i++)); do
	echo "type=AGRIFFIS" >> "$auditd_log$i"
    done
}

function check_keep_logs {
    declare i
    for ((i = 2; i < num_logs * 2 + 1; i++)); do
	if ! grep -Fqx "type=AGRIFFIS" "$auditd_conf.$i"; then
	    echo "check_keep_logs: failed checking $auditd_conf.$i"
	    return 1
	fi
    done
    return 0
}

function pre_halt {
    declare mask_runlevel=${1:-0}
    old_runlevel=$(runlevel | cut -d' ' -f2)

    # prepend to the cleanup function because it's very important this
    # happens...
    eval "function cleanup {
	if [[ -s /etc/inittab.agriffis ]]; then
	    mv /etc/inittab.agriffis /etc/inittab
	fi
	init $old_runlevel
	$(type cleanup | sed '1,3d')"

    # now remove the runlevel from inittab
    sed -i.agriffis "/:$mask_runlevel:/s/^/#/" /etc/inittab
    init q
}

function check_halt {
    declare want_runlevel=${1:-0}
    declare runlevel=$(runlevel) || return 2
    if [[ $runlevel == *" $want_runlevel" ]]; then
	echo "Great, the runlevel changed to $want_runlevel"
	return 0
    else
	echo "Failed to change runlevels"
	return 1
    fi
}

function pre_single {
    pre_halt 1
}

function check_single {
    check_halt 1
}

function pre_email {
    if [[ -f /var/mail/eal ]]; then
	eal_mail_lines=$(wc -l < "$eal_mail")
    else
	eal_mail_lines=0
    fi
}

function check_email {
    sleep 2	# wait for the mail to arrive
    if [[ ! -f "$eal_mail" ]]; then
	echo "check_email: "$eal_mail" does not exist"
	return 2
    fi
    if sed "1,${eal_mail_lines}d" "$eal_mail" | \
	    grep -m1 '^Subject: Audit Disk Space Alert'; then
	echo "check_email: found out magic email"
	return 0
    else
	echo "check_email: email never arrived"
	return 1
    fi
}

######################################################################
# common startup
######################################################################

trap "cleanup; exit" 0 1 2 3 15

# clean slate
auditctl -D
service auditd stop ||:

# use 8MB tmpfs for audit logs
mount -t tmpfs -o size=$((1024 * 1024 * 8)) none /var/log/audit

# default config ignores all problems
write_auditd_conf \
    log_file=/var/log/audit/audit.log \
    log_format=RAW \
    flush=SYNC \
    num_logs=5 \
    max_log_file=1024 \
    max_log_file_action=IGNORE \
    action_mail_acct=eal \
    space_left=2 \
    space_left_action=IGNORE \
    admin_space_left=1 \
    admin_space_left_action=IGNORE \
    disk_full_action=IGNORE \
    disk_error_action=IGNORE
