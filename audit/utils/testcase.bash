#!/bin/bash
# =============================================================================
# (c) Copyright Hewlett-Packard Development Company, L.P., 2006
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
# testcase.bash: routines available to bash test cases
#
# NB: these should simply echo/printf, not msg/vmsg/dmsg/prf because
#     run_test output is already going to the log.

source functions.bash

######################################################################
# global vars
######################################################################

tmp1=$(mktemp) || exit 2
tmp2=$(mktemp) || exit 2

######################################################################
# utility functions
######################################################################

# This can be prepended or appended by calling prepend_cleanup or append_cleanup
# below
function test_cleanup {
    rm -f "$tmp1" "$tmp2"
}

# can override to cleanup &>/dev/null when appropriate
trap 'test_cleanup; exit' 0 1 2 15

function prepend_cleanup {
    eval "function test_cleanup {
	$*
	$(type test_cleanup | sed '1,3d;$d')
    }"
}

function append_cleanup {
    eval "function test_cleanup {
	$(type test_cleanup | sed '1,3d;$d')
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
