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
#  FILE   : test_cron_nonroot_fail.bash
#
#  TEST DESCRIPTION: Test that cron root-only functionality is not allowed
# 		     for a non-root user.  Test this by trying to execute
#              	     crontab -u root as a normal user.  This should be denied.
#
#  HISTORY:  05/2007  created by Lisa Smith <lisa.m.smith@hp.com>
#	     09/2011  modified by T.N. Santhosh <santhosh.tn@hp.com>
#            11/2011  modified by T.N. Santhosh <santhosh.tn@hp.com>
#
#############################################################################

source testcase.bash || exit 2
source cron_functions.bash || exit 2

# Prepare environment for test run
echo "***** Starting cron_nonroot_fail test ******"
backup_files
cleanup
test_prep

# An empty cron.deny file is needed to allow a non-root user to run crontab
touch $CRON_DENY

echo "TEST THAT CRONTAB -U ROOT CAN NOT BE EXECUTED BY A NON-ROOT USER"

# Try to run with the -u option as a non-root user
/bin/su $TEST_USER -c "crontab -u root << EOF
* * * * * id >> $TEST_DIR/output_cron 2>&1
EOF"

# Verify the crontab command was not allowed
if [ $? != 0 ]; then
	cleanup
	exit_pass
else
	cleanup
	exit_fail "crontab -u allowed for a non-root user"
fi
