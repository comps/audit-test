#!/bin/bash
# ==========================================================================
#   (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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
# ==========================================================================
#
# FILE:  cron_functions.bash
#
# DESCRIPTION:  Helper functions available to cron test cases
#
# HISTORY:  04/2007  created by Lisa Smith <lisa.m.smith@hp.com>
# HISTORY:  11/2011  Modified by T N Santhosh <santhosh.tn@hp.com>
#
##############################################################################

TEST_USER=$TEST_ADMIN
TEST_USER2="eal2"
TEST_DIR="$(pwd)/tmp"
CRON_ALLOW="/etc/cron.allow"
CRON_DENY="/etc/cron.deny"

function backup_files {
      # taking backup of cron.allow and cron.deny files before starting the test.

      if [ -e $CRON_ALLOW ]; then
        backup $CRON_ALLOW
      fi

      if [ -e $CRON_DENY ]; then
        backup $CRON_DENY
      fi
}

function test_prep {

	mkdir $TEST_DIR

        # Ensure tmp directory has the correct context/perms
        chmod 777 $TEST_DIR
        chcon -u system_u -t user_tmp_t -l SystemLow-SystemHigh $TEST_DIR
}

function cleanup {
        # Delete crontabs and tmp file
        if test -e /var/spool/cron/$TEST_USER; then
                crontab -r -u $TEST_USER
        fi

	killall -9 -u $TEST_USER2
	userdel -rf $TEST_USER2 2>/dev/null

	rm -rf $TEST_DIR
	rm -f $CRON_ALLOW $CRON_DENY
}
