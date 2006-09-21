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
#
# functions.bash: routines available to run_test and to bash test cases
#
# NB: these should simply echo/printf, not msg/vmsg/dmsg/prf because
#     run_test output is already going to the log.

[[ -z ${_FUNCTIONS_BASH} ]] || return 0
_FUNCTIONS_BASH=1

shopt -s extglob

######################################################################
# global vars
######################################################################

auditd_conf=/etc/audit/auditd.conf
auditd_orig=$(mktemp $auditd_conf.XXXXXX) || exit 2
useradd_conf=/etc/default/useradd
useradd_orig=$(mktemp $useradd_conf.XXXXXX) || exit 2
audit_log=/var/log/audit/audit.log

# get recipient of root mail from /etc/aliases "root: jdoe, jsmith" line
ralias=$(sed -n 's/^root:[ \t]*\([^,]*\).*/\1/p' /etc/aliases)
[[ -z $ralias ]] && ralias=root
eal_mail=/var/mail/$ralias
unset ralias

messages=/var/log/messages
tmp1=$(mktemp) || exit 2
tmp2=$(mktemp) || exit 2
zero=${0##*/}

######################################################################
# utility functions
######################################################################

# This can be prepended or appended by calling prepend_cleanup or append_cleanup
# below
function cleanup {
    if [[ -s $auditd_orig ]]; then 
        auditctl -D
        service auditd stop
        killall auditd
        mv "$auditd_orig" "$auditd_conf"
        umount /var/log/audit
        service auditd start
    fi
    rm -f "$auditd_orig" "$tmp1" "$tmp2"
}

# can override to cleanup &>/dev/null when appropriate
trap 'cleanup; exit' 0 1 2 15

function prepend_cleanup {
    eval "function cleanup {
	$*
	$(type cleanup | sed '1,3d;$d')
    }"
}

function append_cleanup {
    eval "function cleanup {
	$(type cleanup | sed '1,3d;$d')
	$*
    }"
}

function exit_pass {
    [[ -n $* ]] && echo "pass: $*" >&2
    # this will call the cleanup function automatically because of trap 0
    exit 0
}

function exit_fail {
    [[ -n $* ]] && echo "fail: $*" >&2
    # this will call the cleanup function automatically because of trap 0
    exit 1
}

function exit_error {
    [[ -n $* ]] && echo "error: $*" >&2
    # this will call the cleanup function automatically because of trap 0
    exit 2
}

# backup files, with automatic restore when the script exits.
# prepend_cleanup is used since files should probably be restored in reverse
# order.
function backup {
    declare f b
    for f in "$@"; do
	b=$(mktemp "$f.XXXXXX") || exit_error
	cp -a "$f" "$b" || exit_error
	prepend_cleanup "mv -f '$b' '$f'"
    done
}

######################################################################
# auditd functions
######################################################################

# initialize auditd.conf for testsuite
# called from run.bash
function initialize_auditd_conf {

    # Save off the auditd configuration
    if [[ ! -s "$auditd_orig" ]]; then 
	cp -a "$auditd_conf" "$auditd_orig"
    fi

    # remove configuration for audit dispatcher
    sed -i 's/^[Dd][Ii][Ss][Pp].*//' "$auditd_conf"
}

# the following functions for writing conf files should eventually be refactored
# into a common function.
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
}

function write_useradd_conf {
    declare x key value

    ls /etc/default

    # Save off the auditd configuration
    if [[ ! -s $useradd_orig ]]; then 
	cp -a -v "$useradd_conf" "$useradd_orig"
    fi

    # Replace configuration lines; slow but works
    for x in "$@"; do
	key=${x%%=*}
	value=${x#*=}
	sed -i "/^$key[[:blank:]]*=/d" "$useradd_conf"
	echo "$key=$value" >> "$useradd_conf"
    done
}

function start_auditd {
    declare i s="starting auditd $$: can you hear me now?"
    if ! killall -0 auditd &>/dev/null; then
	# auditd -f lets us see error messages on stdout
	service auditd start &>/dev/null || { auditd -f; return 2; }
    fi

    # auditd daemonizes before it is ready to receive records from the kernel.
    # make sure it's receiving before continuing.
    echo -n "start_auditd: Waiting for auditd to start"
    for ((i = 0; i < 100; i++)); do
	auditctl -m "$s"
	if tail -n10 /var/log/audit/audit.log | grep -Fq "$s"; then
	    echo
	    return 0
	fi
	echo -n .
	sleep 0.1
    done

    echo
    echo "start_auditd: auditd slow starting, giving up" >&2

    return 0
}

function stop_auditd {
    declare i

    auditctl -D &>/dev/null
    service auditd stop || killall auditd
    killall -0 auditd &>/dev/null || return 0

    for ((i = 0; i < 100; i++)); do
	echo -n "stop_auditd: Waiting for auditd to stop"
	if ! killall -0 auditd &>/dev/null; then
	    echo
	    return 0
	fi
	echo -n .
	sleep 0.1
    done

    echo
    echo "stop_auditd: timed out, killing auditd" >&2
    killall -9 auditd

    return 0
}

# This function is generally called from 
function rotate_audit_logs {
    declare tmp num_logs

    if [[ -f /var/log/audit/audit.log ]]; then
        pushd /var/log/audit >/dev/null
        tmp=$(mktemp $PWD/rotating.XXXXXX) || return 2
        ln -f audit.log "$tmp" || return 2

	# Attempt to rotate using mechanism available in 1.0.10+
	echo "rotate_audit_logs: Attempting to rotate using USR1"
	if killall -0 auditd &>/dev/null; then
	    killall -USR1 auditd &>/dev/null
	    sleep 0.1
	fi

        # If rotation didn't work, do it manually.
        if [[ audit.log -ef $tmp ]]; then
	    echo "rotate_audit_logs: Seems that USR1 is not supported"
            stop_auditd
            num_logs=$(awk '$1=="num_logs"{print $3;exit}' $auditd_conf)
            while ((--num_logs > 0)); do
                if ((num_logs == 1)); then
		    [[ -f audit.log ]] && mv -f audit.log audit.log.1
                else
		    [[ -f audit.log.$((num_logs-1)) ]] && \
			mv -f audit.log.$((num_logs-1)) audit.log.$((num_logs))
                fi
            done
        fi

        rm -f "$tmp"
        popd >/dev/null
    fi

    killall -0 auditd &>/dev/null || start_auditd
}
