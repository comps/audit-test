#!/bin/bash
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

export PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin
if [[ -z $TOPDIR ]]; then
    TOPDIR=$(
	while [[ ! $PWD -ef / ]]; do
	    [[ -f rules.mk ]] && { echo $PWD; exit 0; }
	    cd ..
	done
	exit 1
    ) || { echo "Can't find TOPDIR, where is rules.mk?" >&2; exit 2; }
    export TOPDIR
fi
PATH=$TOPDIR/utils:$PATH

source testcase.bash

######################################################################
# global variables
######################################################################

# get recipient of root mail from /etc/aliases "root: jdoe, jsmith" line
ralias=$(sed -n 's/^root:[ \t]*\([^,]*\).*/\1/p' /etc/aliases)
[[ -z $ralias ]] && ralias=root
eal_mail=/var/mail/$ralias
unset ralias

messages=/var/log/messages
action=$1
total_written=0

######################################################################
# common functions
######################################################################

function write_records {
    declare max=$1 i
    echo "Writing records to audit log ($max)"
    for ((i=1; i<=max; i++)); do
	#auditctl -m "$zero $action $i" || return 2
	auditctl -r 0 >/dev/null || return 2 # XXX 
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

# write_file(path, kb): writes a file of kb size, with content resembling an
# audit log
function write_file {
    declare path=$1 kb=$2
    echo "Writing $path of kb $kb KB"
    kb=$kb perl -e '
	$mystring = sprintf "%-1023s\n", "type=AGRIFFIS";
	for ($x = 0; $x < $ENV{kb}; $x++) { print $mystring }' \
	    >"$path"
    ls -lk "$path"
}

# fill_disk(dir, free): creates a file in dir which fills the filesystem,
# leaving free kb available
function fill_disk {
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

function check_rotate {
    if [[ ! -s "$audit_log.1" ]]; then
	echo "check_rotate: log did not rotate"
	return 1
    fi

    # XXX uncomment when policy updated for auditctl -m
    #echo "Verifying the logs were written correctly (no missing or duped)"
    #grep -aho "$zero $action [[:digit:]]*" "$audit_log.1" "$audit_log" >"$tmp1"
    #seq 1 $total_written | sed "s/^/$zero $action /" >"$tmp2"
    #diff "$tmp1" "$tmp2" || return 1
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
    declare strace_pid seconds

    # strace-4.5.13-0.EL4.1, at least on em64t and ia64, has double-free issues
    # *on exit* when tracing auditd.  This doesn't seem to affect our use of the
    # tool, so work around the problem (which would by default abort strace)
    # with MALLOC_CHECK_=0.  See http://dag.wieers.com/howto/compatibility/
    prepend_cleanup "kill $strace_pid 2>/dev/null" # for ctrl-c
    MALLOC_CHECK_=0 strace -p $(pidof auditd) -ff -e trace=write -s 1024 -o "$tmp1" &
    strace_pid=$!
    sleep 1	# wait for strace to get started

    write_records 10
    auditctl -m "$zero $$"
    write_records 10

    # in the disk_full_action test, it can take a while for strace to
    # see all of the writes
    for ((seconds = 60; seconds > 0; seconds--)); do
	# detect if policy killed our strace
	if ! kill -0 $strace_pid; then
	    # can't search with augrok because the log is full...
	    exit_error "AVC blocked strace auditd"
	fi
	if grep -Fm1 "$zero $$" "$tmp1"; then
	    echo "check_ignore: cool, auditd is still attempting to write"
	    echo "waited $((60 - seconds)) seconds to find magic write"
	    kill $strace_pid
	    return 0
	fi
	sleep 1
    done
    kill $strace_pid
    echo "check_ignore: it appears auditd is suspended:"
    cat "$tmp1"
    return 1
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

    append_cleanup '
    if [[ -s /sbin/init.agriffis ]]; then
	mv /sbin/init.agriffis /sbin/init
    fi'

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
	if augrok -q type=AVC extra_text='avc: denied { write } for' comm=init 'name=~tmp'; then
	    exit_error "AVC blocked /sbin/init writing $tmp1"
	fi
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
    if [[ -f $eal_mail ]]; then
	eal_mail_lines=$(wc -l < "$eal_mail")
    else
	eal_mail_lines=0
    fi
}

function check_email {
    sleep 2	# wait for the mail to arrive

    if augrok -q type=AVC comm=auditd 'name=~sendmail'; then
	exit_error "AVC blocked auditd running sendmail"
    fi

    if [[ ! -f $eal_mail ]]; then
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

if [[ -z $action ]]; then
    echo "Please specify a variation on the command-line"
    exit 2
fi

# clean slate
stop_auditd

# use 8MB tmpfs for audit logs
if mount | grep /var/log/audit; then exit 2; fi
mount -t tmpfs -o size=$((1024 * 1024 * 8)) none /var/log/audit
# fix permissions for temporary audit logs
# FIXME: chcon should be conditionalized based on the presence of selinux
chmod 750 /var/log/audit
chcon system_u:object_r:auditd_log_t /var/log/audit

prepend_cleanup '
    umount /var/log/audit
    service auditd start'
backup "$auditd_conf"	# restore done via prepend_cleanup
prepend_cleanup '
    service auditd stop
    killall auditd'

# default config ignores all problems
write_config -s "$auditd_conf" \
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
    write_config -s "$auditd_conf" action_mail_acct=root
fi

if [[ $(type -t pre_$action) == function ]]; then
    pre_$action
fi

prepend_cleanup '{
    echo
    echo "Audit log snippet"
    echo "--- start audit.log --------------------------------------------------------"
    augrok type!=AGRIFFIS not \( type=CONFIG_CHANGE audit_rate_limit=0 \)
    echo "--- end audit.log ----------------------------------------------------------"
}'
