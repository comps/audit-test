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
#  FILE   : test_cron_set_valid_mls_level.bash
#
#
#  TEST DESCRIPTION: Test that the basic cron job functionality works as
#                    expected. Create a crontab as a normal user and add a job.
#                    Verify the correct execution of the job and delete crontab.
#
#            09/2011   modified by T.N Santhosh <santhosh.tn@hp.com>
#            11/2011   modified by T.N Santhosh <santhosh.tn@hp.com>
#
#############################################################################

source testcase.bash || exit 2
source cron_functions.bash || exit 2

# be verbose
set -x

DEF_SEC_LEVEL_sysadm_u="SystemLow-SystemHigh"
DEF_SEC_LEVEL_staff_u="SystemLow-SystemHigh"
DEF_SEC_LEVEL_user_u="SystemLow"
TEST_SEC_LEVEL_sysadm_u="Secret"
TEST_SEC_LEVEL_staff_u="Secret"
# process running as user_u has clearance level only SystemLow at most
TEST_SEC_LEVEL_user_u="SystemLow"

# check if mcstransd service running
service mcstransd status || exit_error "mcstransd is not running"

# Prepare environment for test run
echo "***** Starting cron_set_valid_mls_level test ******"
backup_files
cleanup
test_prep

# An empty cron.deny file is needed to allow a non-root user to run crontab
touch $CRON_DENY

# Try out the cron scenario for user_u, sysadm_u and staff_u users
for SUSER in staff_u user_u; do

        # Expected default level
        DEF_SEC_LEVEL=$(eval echo "\$DEF_SEC_LEVEL_$SUSER")

        # Clearance level of the crond process running user's job
        TEST_SEC_LEVEL=$(eval echo "\$TEST_SEC_LEVEL_$SUSER")

        # Set SELinux user with default level
        semanage login -d $TEST_USER
        semanage login -a -s $SUSER $TEST_USER

        # Check if default level correct
        semanage login -l | egrep "${TEST_USER}.*${SUSER}.*$DEF_SEC_LEVEL"
        if [ $? -ne 0 ]; then
            semanage login -l
            cleanup
            exit_fail "SELinux login record for $TESTUSER is wrong (${TEST_USER}.*${SUSER}.*$DEF_SEC_LEVEL)"
        fi

        # In order to create a file at a level higher than the lowest level
        # allowed by the range of /tmp, create a directory at the level of which
        # we want to create the file
        chcon -u system_u -t user_tmp_t -l $TEST_SEC_LEVEL $TEST_DIR

        # Add new job
        /bin/su $TEST_USER -c "crontab - << EOF
MLS_LEVEL=$TEST_SEC_LEVEL
* * * * * id >> $TEST_DIR/output_cron 2>&1
EOF"

        # Verify the crontab was successfully added
        if [ $? != 0 ]; then
                cleanup
                exit_fail "Error while adding crontab for user $TEST_USER ($SUSER)"
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
            exit_fail "Cron did not execute job ($SUSER)"
        else
            ls -Z $TEST_DIR/output_cron | grep $TEST_SEC_LEVEL
            if [ $? = 0 ]; then
                echo "Job has been executed at the correct MLS_LEVEL specified ($SUSER)"
                rm -f $TEST_DIR/output_cron
            else
                cleanup
                exit_fail "Job did not execute at the specified MLS_LEVEL ($SUSER)"
            fi
        fi
done

cleanup
exit_pass "cron_set_valid_mls_level"
