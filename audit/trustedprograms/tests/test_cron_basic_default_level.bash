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
#  FILE   : test_cron_basic_pass.bash
#
#  TEST DESCRIPTION: Test that the basic cron job functionality works as
# 		     expected. Create a crontab as a normal user and add a job.
#              	     Verify the correct execution of the job and delete crontab.
#
#  HISTORY:  04/2007  created by Lisa Smith <lisa.m.smith@hp.com>
#            11/2011  Modified by T N Santhosh <santhosh.tn@hp.com>
#
#############################################################################

source testcase.bash || exit 2
source cron_functions.bash || exit 2

DEF_SEC_LEVEL="SystemLow-SystemHigh"

# Prepare environment for test run
echo "***** Starting cron_basic_pass test ******"
backup_files
cleanup
test_prep

# An empty cron.deny file is needed to allow a non-root user to run crontab
touch $CRON_DENY

# Add new job
/bin/su $TEST_USER -c "crontab - << EOF
* * * * * id >> $TEST_DIR/output_cron 2>&1
EOF"

# Verify the crontab was successfully added
if [ $? != 0 ]; then
	cleanup
	exit_fail "Error while adding crontab for user $TEST_USER"
fi

# Change label and owner so crond executes properly
chcon -t user_cron_spool_t /var/spool/cron/$TEST_USER
echo "New job added successfully"

# Wait for execution of job
echo "sleeping for up to 70 seconds..."
rc=1
for ((i=0; i<70; i++)); do
    if test -e $TEST_DIR/output_cron; then
        rc=0
        break
    fi
    sleep 1
done

if [ $rc = 1 ]; then
	cleanup
        exit_fail "Cron did not execute job"
else
        ls -Z $TEST_DIR/output_cron | grep $DEF_SEC_LEVEL
        if [ $? != 0 ]; then
                echo "Job has been executed at the correct default MLS level"
		cleanup
                exit_pass "cron_mls_default_level"

        else
		cleanup
                exit_fail "Job did not execute at the user's default MLS level"
        fi
fi
