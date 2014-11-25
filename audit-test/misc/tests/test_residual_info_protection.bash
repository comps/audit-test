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
# SFRs: FDP_RIP.2
#
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
#
# DESCRIPTION:
#  Ensure that deleted information is no longer accessible, and that newly-created
#  objects do not contain information from previously used objects within the TOE.
#
# TESTS:
#  fs - test all available filesystems (using sparse-file.c)
#

source testcase.bash

# test all available file systems via sparse file
test_fs() {
    local FSTYPES=$(mount | grep rw | sed 's|.*type \([[:alnum:]]*\).*|\1|g' | \
        sort | uniq | egrep -v "(binfmt|autofs)")
    local FS= TPATH=
    local EXIT_MSGS=

    for FS in $FSTYPES; do
        TPATH=$(mount | grep -m1 " $FS.*rw" | awk '{print $3}')
        echo ":: Testing filesystem '$FS' on mount point '$TPATH'"
        ./sparse_file $TPATH; RET=$?
        case $RET in
            255) EXIT_MSGS=$(printf "%s\n%s" "$EXIT_MSGS" "Sparse files are not correct on $FS filesystem") ;;
            1|3) echo "Filesystem is not writable, skipping" ;;
            5) echo "Filesystem seems full, skipping" ;;
            # just do nothing if test passes
            0) : ;;
            *) exit_error "Unexpected error ($RET)" ;;
        esac
    done

    if [ -n "$EXIT_MSGS" ]; then
        echo "Found these failures:"
        echo "$EXIT_MSGS"
        exit_fail "Some filesystems show sparse files issues"
    fi
}

[ -z "$1" ] && exit_error "No test sepcified"

# execute function cmd_$PARAM according to given positional parmater $PARAM
for TST in $@; do
    # execute test function if exists
    if [ "$(type -t test_$TST)" = "function" ]; then
        eval test_$TST
    else
        printf "Error: unknown test $TST\n"
        help
    fi
done

exit_pass

# vim: ts=4 sw=4 sts=4 ft=sh et ai:
