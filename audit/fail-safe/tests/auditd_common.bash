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
    ls -l ${audit_log}*
    df -k ${audit_log%/*}/
}

# write_file(path, kb): writes a file of kb size, with content resembling an
# audit log
function write_file {
    declare path=$1 kb=$2
    echo "Writing $path of kb $kb KB"
    kb=$kb perl -e '
	$mystring = sprintf "%-1023s\n", "type=AGRIFFIS";
	for ($x = 0; $x < $ENV{kb}; $x++) { print $mystring }' \
	    >"$path"
    ls -s --block-size=1 "$path"
}

# fill_disk(dir, free): creates a file in dir which fills the filesystem,
# leaving free kb available
function fill_disk {
    # So far, this only happens on tmpfs, so don't worry about deleting the temp
    # file later.
    declare dir=$1 free=$2
    echo "Filling $dir, leaving $free KB free"
    dd if=/dev/zero of="$dir/bogus" bs=1024 count=$free
    cat </dev/zero >"$dir/bloat" ||:
    rm -f "$dir/bogus"
    df -k $dir/
}

function cleanup {
    echo "Audit log snippet"
    echo "--- start audit.log --------------------------------------------------------"
    augrep 'type!=AGRIFFIS' ||:
    echo "--- end audit.log ----------------------------------------------------------"
    auditctl -D ||:
    service auditd stop ||:
    killall auditd ||:
    if [[ -s "$auditd_orig" ]]; then mv "$auditd_orig" "$auditd_conf"; fi
    rm -f "$auditd_orig" "$tmp1" "$tmp2"
    umount /var/log/audit ||:
    service auditd start ||:
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
    messages_lines=$(wc -l </var/log/messages)
    logger "making sure syslog works $$"
}

function check_syslog {
    declare syslog auditd_pid=$(pidof auditd | cut -f1 -d\ )

    sleep 0.1	# let syslog catch up
    syslog=$(sed "1,${messages_lines}d" "$messages")

    if grep -Fq " auditd[$auditd_pid]: $1" <<<"$syslog"; then
	return 0
    fi

    echo "--- start messages ---------------------------------------------------------"
    echo "$syslog"
    echo "--- end messages -----------------------------------------------------------"

    if grep -Fq "making sure syslog works $$" <<<"$syslog"; then
	echo "check_syslog: couldn't find auditd syslog record"
	return 1
    else
	echo "check_syslog: syslog appears to be broken"
	return 2
    fi
}

function check_ignore {
    declare strace_pid

    # strace-4.5.13-0.EL4.1, at least on em64t and ia64, has double-free issues
    # *on exit* when tracing auditd.  This doesn't seem to affect our use of the
    # tool, so work around the problem (which would by default abort strace)
    # with MALLOC_CHECK_=0.  See http://dag.wieers.com/howto/compatibility/
    MALLOC_CHECK_=0 strace -p $(pidof auditd) -ff -e trace=write -s 1024 -o "$tmp1" &
    strace_pid=$!
    sleep 1	# wait for strace to get started
    write_records 10
    auditctl -m "$zero $$"
    write_records 10
    sleep 1	# make sure strace is up to date
    kill $strace_pid

    if grep -Fm1 "$zero $$" "$tmp1"; then
	echo "check_ignore: cool, auditd is still attempting to write"
	return 0
    else
	echo "check_ignore: it appears auditd is suspended:"
	cat "$tmp1"
	return 1
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
    num_logs=$(awk '/^num_logs =/{print $NF;exit}' "$auditd_conf")
    declare i
    for ((i = 1; i < num_logs * 2; i++)); do
	echo "type=AGRIFFIS" >> "$audit_log.$i"
    done
}

function check_keep_logs {
    declare i
    for ((i = 2; i < num_logs * 2 + 1; i++)); do
	if ! grep -Fqx "type=AGRIFFIS" "$audit_log.$i"; then
	    echo "check_keep_logs: failed checking $audit_log.$i"
	    return 1
	fi
    done
    return 0
}

function pre_halt {
    declare mask_runlevel=${1:-0}

    eval "function cleanup {
	$(type cleanup | sed '1,3d;$d')
	if [[ -s /sbin/init.agriffis ]]; then
	    mv /sbin/init.agriffis /sbin/init
	fi
    }"

    # replace /sbin/init with our own version
    if [[ ! -s /sbin/init.agriffis ]]; then
	mv /sbin/init /sbin/init.agriffis
    fi

    cat >/sbin/init <<EOF
#!/bin/bash -x
if [[ \$1 == $mask_runlevel ]]; then
    echo \$1 > "$tmp1"
else
    exec /sbin/init "\$@"
fi
EOF
    chmod +x /sbin/init
}

function check_halt {
    declare want_runlevel=${1:-0}
    sleep 0.1	# make sure auditd had a chance to call /sbin/init
    if [[ $(<$tmp1) == $want_runlevel ]]; then
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
    if sed "1,${eal_mail_lines}d" "$eal_mail" | grep -Fq "$1"; then
	echo "check_email: found magic email"
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

zero=${0##*/}
action=$1

# clean slate
auditctl -D
service auditd stop || killall auditd ||:

# use 8MB tmpfs for audit logs
if mount | grep /var/log/audit; then exit 2; fi
mount -t tmpfs -o size=$((1024 * 1024 * 8)) none /var/log/audit

# default config ignores all problems
write_auditd_conf \
    log_file=/var/log/audit/audit.log \
    log_format=RAW \
    flush=SYNC \
    num_logs=5 \
    max_log_file=1024 \
    max_log_file_action=IGNORE \
    space_left=2 \
    space_left_action=IGNORE \
    admin_space_left=1 \
    admin_space_left_action=IGNORE \
    disk_full_action=IGNORE \
    disk_error_action=IGNORE

# email actions aren't fully available until version 1.0.8, so don't write
# this config item unless that's what we're testing.
if [[ $action == email ]]; then
    write_auditd_conf action_mail_acct=eal
fi

if [[ $(type -t pre_$action) == function ]]; then
    pre_$action
fi
