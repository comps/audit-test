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
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
#
# DESCRIPTION:
# Test MLS specific functionality of DBus component.
#
# For detailed description of the test scenarios see the test_* functions.
#
# For more information see this bugzilla:
# https://bugzilla.redhat.com/show_bug.cgi?id=1118399
#

source testcase.bash || exit 2
source tp_dbus_functions.bash || exit 2

# be verbose
set -x

TEST_RANGE="s5-s6"

# check if dbus test server connection present in listing
listing_present() {
    runcon -l $1 dbus-send --print-reply --system --dest=org.freedesktop.DBus \
        / org.freedesktop.DBus.ListNames | grep -q $DBUS_IF
}

#
# Test: Test if listing DBus connections work if the user
#       is in security level range
#
# The context is checked via this constraint
# mlsconstrain context contains (( h1 dom h2 ) and ( l1 domby l2));
#
test_list_in_range() {
    local LEVEL=

    # Pass cases
    # eg for s4-s7: s7 (h1) dominates s6 (h2), s4 (l1) is dominated by s5 (l2)
    for LEVEL in s4-s7 s5-s6 s3-s6 s5-s7; do
        listing_present $LEVEL || exit_fail \
            "$DBUS_IF was not found with sufficient security level $LEVEL"
    done

    # Fail cases
    for LEVEL in s0 s5 s6 s8 s4-s5; do
        listing_present $LEVEL && \
            exit_fail "$DBUS_IF found unexpectedly with $LEVEL security level"
    done
}

# create default configuration
dbus_create_config

# start test server on TEST_RANGE
dbus_server_start $TEST_RANGE

# run testing
if [ "$(type -t test_$1)" = "function" ]; then
    eval test_$1
else
    help
fi

exit_pass

# vim: ts=4 sw=4 sts=4 ft=sh et ai:
