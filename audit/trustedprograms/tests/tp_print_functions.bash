#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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
###############################################################################

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
        
    /usr/sbin/lpadmin -p $PRINTER -E -v parallel:/$PRINTERDEV -m postscript.ppd.gz
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

    /usr/sbin/lpadmin -p $PRINTER -E -v socket://localhost:$PRINTERPORT -m postscript.ppd.gz
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
        SECONDS=7
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
    # if everything appears to be right, assume it is
    /sbin/service cups status
    if [ $? -eq 0 \
         -a $(grep -i ^Classification /etc/cups/cupsd.conf | awk '{print $2}') == mls \
         -a $(grep -i ^PerPageLabels /etc/cups/cupsd.conf | awk '{print $2}') == yes ]; then
        return
    fi

    backup /etc/cups/cupsd.conf
    prepend_cleanup 'expect -c "spawn /usr/sbin/run_init /etc/init.d/cups restart \
      expect { \
        -nocase \"password: \" {send \"$PASSWD\\r\"; exp_continue} \
        eof \
      }"'

    sed -ie "s/Classification.*/Classification mls/" /etc/cups/cupsd.conf
    sed -ie "s/.*PerPageLabels.*/PerPageLabels no/" /etc/cups/cupsd.conf

    expect -c "
      spawn /usr/sbin/run_init /etc/init.d/cups restart
      expect {
        -nocase \"password: \" {send \"$PASSWD\\r\"; exp_continue}
        eof
      }"

    /sbin/service cups status || exit_error "cupsd is not running"

}
