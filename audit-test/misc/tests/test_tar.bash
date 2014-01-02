#!/bin/bash
# ==========================================================================
#   (C) Copyright Hewlett-Packard Development Company, L.P., 2005
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
#
#
#  FILE   : test_tar.bash
#
#  TEST DESCRIPTION: Verify that the tar program preserves file security
#		     (SELinux) contexts and ACLs. Pack up files with various
#		     attributes using tar, unpack them in another directory
#		     and compare the file contexts and ACLs. Both the file
#		     contexts and ACLs should all be preserved.
#
#  HISTORY:  05/2007  created by Lisa Smith <lisa.m.smith@hp.com>
#            08/2011  ported to audit-test by Tony Ernst <tee@sgi.com>
#            10/2013  added ACL testing by Jiri Jaburek <jjaburek@redhat.com>
#
#############################################################################
source misc_functions.bash || exit 2

TAR_FILE="labeled.tar"
FILE_DIR="test_files"
EXTRACT_DIR="extract_dir"

# Prepare environment for test run
tar_cleanup

# Clean up at test exit
prepend_cleanup tar_cleanup

# Create test files
mkdir -m 0755 $FILE_DIR $EXTRACT_DIR
echo "This file is at level SystemHigh." > $FILE_DIR/fileHigh
echo "This file is at level SystemLow." > $FILE_DIR/fileLow
echo "This file is at level SystemSecret." > $FILE_DIR/fileSecret

# Ensure the test files have the proper permissions/context
chmod 644 $FILE_DIR/fileHigh || exit_fail
chcon -t var_t -l SystemHigh $FILE_DIR/fileHigh || exit_fail
chmod 755 $FILE_DIR/fileLow || exit_fail
chcon -t tmp_t -l SystemLow $FILE_DIR/fileLow || exit_fail
chmod 744 $FILE_DIR/fileSecret || exit_fail
chcon -t bin_t -l Secret $FILE_DIR/fileSecret || exit_fail

# Add some ACL entries to one of the files
setfacl -n -m u:1234:rwx $FILE_DIR/fileSecret
setfacl -n -m g:4321:--x $FILE_DIR/fileSecret

# Pack up the files in the test_files directory
tar cf $TAR_FILE --selinux --acls -H posix -C $FILE_DIR .

# Verify the files were successfully packed
if [ $? != 0 ]; then
        exit_error "Error creating tar archive"
fi

# Unpack the files
tar xvf $TAR_FILE --selinux --acls -C $EXTRACT_DIR
if [ $? != 0 ]; then
	exit_error "Error unpacking tar archive"
fi

# Check to make sure the files have the correct labels
ls -Z $FILE_DIR > $FILE_DIR.list
ls -Z $EXTRACT_DIR > $EXTRACT_DIR.list
diff $FILE_DIR.list $EXTRACT_DIR.list
if [ $? != 0 ]; then
        exit_fail "tar did not preserve correct files and/or security contexts"
fi

# Check ACLs
getfacl $EXTRACT_DIR/fileSecret | grep user:1234:rwx || exit_fail
getfacl $EXTRACT_DIR/fileSecret | grep group:4321:--x || exit_fail

exit_pass
