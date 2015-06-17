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

export TOPDIR=/usr/local/eal4_testing/audit-test
export PATH=$PATH:$TOPDIR
export PATH=$PATH:$TOPDIR/trustedprograms/tests/
export PATH=$PATH:$TOPDIR/utils/

source functions.bash || exit 2
source tp_ipsec_functions.bash || exit 2

# Wrappers.

function call_ipsec_start {
    start_service ipsec
}

function call_ipsec_stop {
    stop_service ipsec
}

function call_ipsec_restart {
    restart_service ipsec
}

function call_ipsec_del_connection {
    ipsec_del_connection $@
}

function call_ipsec_add_connection {
    ipsec_add_connection $@
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
