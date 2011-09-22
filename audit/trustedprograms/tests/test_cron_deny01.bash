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
#  FILE   : test_cron_deny01.bash
#
#  TEST DESCRIPTION: Test the /etc/cron.deny functionality. Create crontab
#                    and add a job for a user that is NOT listed in the
#		     cron.deny file.  This functionality should be allowed.
#                    Verify execution of the job and delete the crontab.
#
#  HISTORY:  05/2007  created by Lisa Smith <lisa.m.smith@hp.com>
#	     09/2011  modified by T.N. Santhosh <santhosh.tn@hp.com>
#
#############################################################################

source testcase.bash || exit2
source cron_functions.bash || exit 2

# Prepare environment for test run
echo "***** Starting cron_deny01 test ******"
test_prep
prepend_cleanup deny_cleanup

# Create 2nd user
useradd -m -g users $TEST_USER2
if [ $? != 0 ]; then
	exit_fail "Could not add test user $TEST_USER2 to system"
fi

echo "$CRON_DENY should only allow those NOT in the file to run cron jobs."

# Create /etc/cron.deny file
echo $TEST_USER2 > $CRON_DENY

# Add new job
/bin/su $TEST_USER -c "crontab - << EOF
* * * * * echo 'TEST JOB RAN' >> $TEST_DIR/output_cron 2>&1
EOF"

# Verify the crontab was successfully added
if [ $? != 0 ]; then
	exit_fail "Error while adding crontab for user $TEST_USER"
fi

# Change label and owner so crond executes properly
chcon -t staff_cron_spool_t /var/spool/cron/$TEST_USER
echo "New job added successfully"

# Wait for execution of job
echo "sleeping for 70 seconds..."
rc=1
for ((i=0; i<70; i++)); do
    if test -e $TEST_DIR/output_cron; then
        rc=0
        break
    fi
    sleep 1
done

if [ $rc = 1 ]; then
	exit_fail "Cron did not allow user NOT in $CRON_DENY to execute job"
else
	exit_pass
fi
