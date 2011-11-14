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
#  FILE   : test_cron_eulr_options.bash
#
#  TEST DESCRIPTION: Test that the -e, -u, -l, and -r crontab options work as
# 		     expected. Edit a crontab specifying the user with -u,
#		     check to crontab contents with -l, and remove the crontab
#		     with -r.
#
#  HISTORY:  05/2007  created by Lisa Smith <lisa.m.smith@hp.com>
#	     09/2011  modified by T.N. Santhosh <santhosh.tn@hp.com>
#            11/2011  modified by T.N. Santhosh <santhosh.tn@hp.com>
#
#############################################################################

source testcase.bash || exit 2
source cron_functions.bash || exit 2

# Prepare environment for test run
echo "***** Starting cron_eulr_options test ******"
cleanup
test_prep

# An empty cron.deny file is needed to allow a non-root user to run crontab
touch $CRON_DENY

# Add new job
echo -e "30 * * * *      echo crontab is running" | crontab -u $TEST_USER -

# Verify the crontab command was successful
if [ $? != 0 ]; then
	exit_fail "Error while adding crontab for user $TEST_USER"
fi

# Check the contents of the crontab file
expect -c "
  spawn crontab -u $TEST_USER -l
  expect {
	\"30 * * * * echo crontab is running\" {exit 1}
  }"

if [ $? != 1 ]; then
	cleanup
	exit_fail "Error with -l crontab option"
fi

# Remove the crontab
crontab -u $TEST_USER -r

# Check that the crontab is not there
if test -e /var/spool/cron/$TEST_USER; then
	cleanup
	exit_fail "Error with -r crontab option"
else
	cleanup
	exit_pass
fi
