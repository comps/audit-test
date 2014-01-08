#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of version 2 the GNU General Public License as
#   published by the Free Software Foundation.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
###############################################################################

source testcase.bash || exit 2

# Optional args
# $1 name for the print queue
# $2 device file to print to
function create_parallel_printer {
    if [ -z $1 ]; then
        PRINTER=testp
    else
        PRINTER=$1
    fi

    if [ -z $2 ]; then
        PRINTERDEV=/dev/parport0
    else
        PRINTERDEV=$2
    fi

    /usr/bin/lpq -P $PRINTER 2> /dev/null && return 0 # bail if the printer already exists

    /usr/sbin/lpadmin -p $PRINTER -E -v parallel:/$PRINTERDEV -m drv:///sample.drv/generic.ppd
}

# Optional args
# $1 name for the print queue
# $2 port to send jobs to
function create_socket_printer {
    if [ -z $1 ]; then
        PRINTER=tests
    else
        PRINTER=$1
    fi

    if [ -z $2 ]; then
        PRINTERPORT=9100
    else
        PRINTERPORT=$2
    fi

    /usr/bin/lpq -P $PRINTER 2> /dev/null && return 0 # bail if printer already exists

    /usr/sbin/lpadmin -p $PRINTER -E -v socket://localhost:$PRINTERPORT -m drv:///sample.drv/generic.ppd
}

# Optional args
# $1 filename to save jobs to
# $2 port to listen for jobs on
# $3 seconds to wait before timeout
function create_socket_listener {
    if [ -z $1 ]; then
        OUTFILE=printjob.out
    else
        OUTFILE=$1
    fi

    if [ -z $2 ]; then
        PRINTERPORT=9100
    else
        PRINTERPORT=$2
    fi

    if [ -z $3 ]; then
        SECONDS=15
    else
        SECONDS=$3
    fi

    /usr/bin/nc -l localhost $PRINTERPORT > $OUTFILE &
    nc_pid=$!

    (sleep $SECONDS; kill $nc_pid; ) &> /dev/null &

    wait $nc_pid
}

# Required - $1 name of the print queue to delete
function delete_printer {
    if [ -z $1 ]; then
        echo "Error: missing printer to delete"
    fi

    lpadmin -x $1
}

function setup_cupsd {
    # if cups is running just return
    /sbin/service cups status && return

    # try to start cups if not running
    restart_service cups

    # cannot start cups = error
    /sbin/service cups status || exit_error "cupsd is not running"
}

# Set next job id to a value
# This makes printing output predictable
function set_next_jobid {
    local JOBCACHE=/var/cache/cups/job.cache

    if [ -z $1 ]; then
        echo "Error: missing printer to delete"
    fi

    # need to stop cups daemon before modifying job cache
    service cups stop

    # restore original job cache after test
    backup $JOBCACHE
    prepend_cleanup "service cups stop"
    append_cleanup "service cups start"

    # modify the job cache and start cups
    sed -i "s/^NextJobId.*/NextJobId $1/" $JOBCACHE
    service cups start
}
