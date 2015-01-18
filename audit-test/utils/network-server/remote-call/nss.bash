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
export PATH=$PATH:$TOPDIR/utils/
export PATH=$PATH:$TOPDIR/crypto/tests/

source functions.bash || exit 2
source tp_nss_functions.bash || exit 2

# Wrappers.

function call_nss_init {
    nss_init $@
}

function call_nss_destroy {
    nss_destroy $@
}

function call_nss_import {
    nss_import $@
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
