#!/bin/bash
###############################################################################
#   Copyright (c) 2014 Red Hat, Inc. All rights reserved.
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
# Remote calls used for IPsec testing. All functions are wrappers for
# function used on TOE, for more details please see documentation of
# original functions.
#
# AUTHOR: Ondrej Moris <omoris@redhat.com>
#

function call_scp {

    expect -c "
        set timeout 60;
        spawn scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null $1 $2;
        expect {
            {*assword:} { send $3\r; exp_continue; }
            {*assword:} { send $3\r; }
            {closed} { exit 1 }
        }
        wait;" || { echo "Cannot copy" ; return 1; }

    return 0
}

# Called function.
action="$1"

shift

# Only call_* function can be called remotely.
if [ "$(type -t call_${action})" = "function" ] ; then
    logger "Received request to execute function call_${action}()"
    call_$action $@
    logger "Executed function call_${action}()"
else
    logger "Failed to execute action ${action}"
    exit 2
fi
