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
# AUTHOR: Ondrej Moris <omoris@redhat.com>
#
# DESCRIPTION: A device labeled with *:device_t:* is mislabeled. This test
#              make sure that there is no such device. Otherwise it fails
#              and reports all mislabeled devices.

source testcase.bash || exit 2

set -x

mislabeled_devices=$(mktemp)

append_cleanup "rm -f $mislabeled_devices"

# Find all character or block special "files" in /dev/ of device_t context.
find /dev -context *:device_t:* \( -type c -o -type b \) -printf "%p %Z\n" \
    > $mislabeled_devices

if [ $(cat $mislabeled_devices | wc -l) -gt 0 ]; then

    echo "The following devices are mislabeled:"
    cat $mislabeled_devices

    exit_fail "Some devices are mislabeled"
fi

exit_pass
