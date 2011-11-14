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
#  FILE   : test_cron_deny02.bash
#
#  TEST DESCRIPTION: Test the /etc/cron.deny functionality. Try to create
#		     a crontab file for a user that is listed in the
#	 	     cron.deny file.  This should not be allowed.
#
#
#  HISTORY:  05/2007  created by Lisa Smith <lisa.m.smith@hp.com>
#	     09/2011  modified by T.N. Santhosh <santhosh.tn@hp.com>
#            11/2011  modified by T.N. Santhosh <santhosh.tn@hp.com>
#
#############################################################################

source testcase.bash || exit 2
source cron_functions.bash || exit 2

# Prepare environment for test run
echo "***** Starting cron_deny02 test ******"
cleanup
test_prep


echo "TEST THAT USER IN $CRON_DENY IS NOT ABLE TO RUN CRONTAB."

# Create /etc/cron.deny file with only $TEST_USER
echo $TEST_USER > $CRON_DENY

# Try to create a crontab and add a job for $TEST_USER
/bin/su $TEST_USER -c "crontab - << EOF
* * * * * echo 'CRON_DENY02 TEST JOB RAN' >> $TEST_DIR/output_cron 2>&1
EOF"

# Verify the crontab was successfully denied
if [ $? != 0 ]; then
	cleanup
	exit_pass
else
	cleanup
	exit_fail "Cron allowed user in $CRON_DENY to execute test job"
fi
