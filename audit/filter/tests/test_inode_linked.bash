#!/bin/bash
###############################################################################
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
###############################################################################
#
# PURPOSE: Test the ability to filter based on device/inode number,
#          specifically for events generated by a hard link or symlink
#          to the audited object.

source filter_functions.bash || exit 2

# setup
op=$1

tmpd=$(mktemp -d) || exit_fail "create tempdir failed"
linkto="$tmpd/link"
case $op in
    link)    ln $tmp1 $linkto ;;
    symlink) ln -s $tmp1 $linkto ;;
    *) exit_fail "unknown test operation" ;;
esac

inode="$(stat -c '%i' $tmp1)"
dev="$(get_fs_dev $tmp1)"
major="$(stat -Lc '%t' $dev)"
minor="$(stat -Lc '%T' $dev)"

auditctl -a exit,always -S open -F key=$tmp1 -F inode=$inode \
    -F devmajor=$major -F devminor=$minor

prepend_cleanup "
    auditctl -d exit,always -S open -F key=$tmp1 -F inode=$inode \
-F devmajor=$major -F devminor=$minor
    rm -rf $tmpd"

log_mark="$(stat -c %s $audit_log)"

# test
cat $linkto >/dev/null

# verify audit record
augrok --seek=$log_mark "type=~SYSCALL" key==$tmp1 \
    || exit_fail "Expected record not found."

exit_pass
