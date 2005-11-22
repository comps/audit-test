#!/bin/bash -e

auditd_conf=/etc/auditd.conf
auditd_orig=$(mktemp $auditd_conf.XXXXXX)
audit_log=/var/log/audit/audit.log
audit_log_1=/var/log/audit/audit.log.1
max_log_file=1  # 1M
tmp1=$(mktemp)
tmp2=$(mktemp)
ctr=0

function startup {
    # Shut down auditing before making config changes
    auditctl -D
    service auditd stop ||:
    rm -f ${audit_log}*

    # Set the max log size
    cp "$auditd_conf" "$auditd_orig"
    grep -vEe '^(log_file|log_format|num_logs|max_log_file|max_log_file_action)[[:blank:]]*=' \
        "$auditd_orig" > "$auditd_conf"
    cat >>$auditd_conf <<-EOF
	log_file = $audit_log
	log_format = RAW
	num_logs = 2
	max_log_file = $max_log_file
	max_log_file_action = ROTATE
	EOF
    echo "auditd.conf log settings:"
    grep log "$auditd_conf"

    # Prepopulate log with max_log_file minus 10k
    max_log_file=$max_log_file perl -e 'print "type=AGRIFFIS" . 
	(" " x (($ENV{max_log_file}*1024-10)*1024)) . "\n"' >"$audit_log"
    stat "$audit_log"

    # Start up auditing with the new configuration
    service auditd start
}

function cleanup {
    auditctl -D
    mv -f "$auditd_orig" "$auditd_conf"
    killall -HUP auditd
    rm -f "$tmp1" "$tmp2"
}

trap "cleanup;exit" 0 1 2 3 15

function write_records {
    echo "Writing records to audit log ($ctr)"
    while true; do
	(( ctr++ ))
	auditctl -m "Testing log rotation $ctr"
	if (( ctr % $1 == 0 )); then break; fi
    done
}

function force_rotate {
    # write batches of 100 message to the log until it rotates
    while [[ ! -f "$audit_log_1" ]]; do
	write_records 100
	if [[ $(stat -c %s "$audit_log") -gt $((max_log_file * 1024 * 1024 * 2)) ]]; then
	    echo "Log never rotated"
	    exit 1
	fi
    done
    echo "Log rotated"
    # write one more batch so there's guaranteed to be some in the new log
    write_records 100
}

function verify {
    grep -aho 'Testing log rotation [[:digit:]]*' "$audit_log_1" "$audit_log" >"$tmp1"
    seq 1 $ctr | sed 's/^/Testing log rotation /' >"$tmp2"
    diff "$tmp1" "$tmp2"
}

startup
force_rotate
verify
