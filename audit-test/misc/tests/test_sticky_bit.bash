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
# STICKY BIT DESCRIPTION (for Linux):
# When the sticky bit is set on a directory, files in that directory may only
# be unlinked or renamed by root or the directory owner or the file owner.
# Sticky bit on files is ignored.
#
# Setup:
# 1. create a temporary directory owned by testadmin:testuser with rwx permissions
#    for all and with sticky bit set (i.e. 1777)
#    note: all commands in testing scenarios are in context of this directory
#
# Testing scenario:
# 1. create a file owned by testuser:testadmin and verify root, testuser and
#    testadmin can rename and remove the file
# 2. create a file owned by root:root and verify testadmin can rename
#    and remove the file. The testuser user is not able to do so.
#

source testcase.bash || exit 2

### helpers
# create_file user:group perms in $TMPDIR
create_file() {
    [ -d "$TMPDIR" ] || exit_error "No temporary directory found"
    [ -z "$1" ] && exit_error "No user:group for $FUNCNAME"
    [ -z "$2" ] && exit_error "No perms for $FUNCNAME"

    local FILE=$(mktemp -p $TMPDIR)
    chown $1 $FILE || exit_error "Could not chown"
    chmod $2 $FILE || exit_error "Could not chmod"

    echo $FILE
}

### main
set -x

### setup
TMPDIR=$(mktemp -d)
append_cleanup "rm -rf $TMPDIR"
chown testadmin:testuser $TMPDIR
chmod 1777 $TMPDIR

### scenario 1
# root can remove or rename file
mv $(create_file testuser:testadmin 000) $(mktemp -p $TMPDIR -u) || \
    exit_fail "root cannot rename test file"
rm -f $(create_file testuser:testadmin 000) || \
    exit_fail "root cannot remove test file"

# file owner can remove or rename file
TFILE=$(create_file testuser:testadmin 000)
OFILE=$(mktemp -p $TMPDIR -u)
su -c "mv $TFILE $OFILE" testuser || \
    exit_fail "owner cannot rename test file"
TFILE=$(create_file testuser:testadmin 000)
su -c "rm -f $TFILE" testuser || \
    exit_fail "owner cannot remove test file"

# directory owner can remove or rename file
TFILE=$(create_file testuser:testadmin 000)
OFILE=$(mktemp -p $TMPDIR -u)
su -c "mv $TFILE $OFILE" testadmin || \
    exit_fail "owner cannot rename test file"
TFILE=$(create_file testuser:testadmin 000)
su -c "rm -f $TFILE" testadmin || \
    exit_fail "owner cannot remove test file"

### scenario 2
# directory owner can rename or remove file
TFILE=$(create_file root:root 000)
OFILE=$(mktemp -p $TMPDIR -u)
su -c "mv $TFILE $OFILE" testadmin || \
    exit_fail "owner cannot rename test file"
TFILE=$(create_file root:root 000)
su -c "rm -f $TFILE" testadmin || \
    exit_fail "owner cannot remove test file"

# other cannot rename or remove file
TFILE=$(create_file root:root 000)
OFILE=$(mktemp -p $TMPDIR -u)
su -c "mv $TFILE $OFILE" testuser && \
    exit_fail "other user could rename test file"
TFILE=$(create_file root:root 000)
su -c "rm -f $TFILE" testuser && \
    exit_fail "other user could remove test file"

exit_pass
