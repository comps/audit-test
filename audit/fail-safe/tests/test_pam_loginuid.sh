#!/bin/bash
# =============================================================================
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
# Written by Matt Anderson <mra@hp.com>
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
# =============================================================================
#
# FILE		: test_pam_loginuid.sh
#
# PURPOSE	: Used to test pam_loginuid and the require_auditd flag
#
# DESCRIPTION	: This test runs through the following steps
#	1. create test user
#	2. have test user login over ssh
#	3. disable auditing
#	4. have test user login again
#	5. show the login failed due to auditing being off
#	6. re-enable auditing
#	7. remove test user
# 
# HISTORY	:
#  11/05 Initial version by Matt Anderson <mra@hp.com>
#

export RHOST="localhost"
export TEST_USER="loginuid_user"
export TEST_USER_PASSWD="eal" # DON'T CHANGE pre-crypted passwd used later
export TEST_USER_HOMEDIR="/home/$TEST_USER"
FIRSTRUN=-1
SECONDRUN=-1

function setup_env {
  # if there is cruft, remove it before making more
  USER_EXISTS=X`grep $TEST_USER /etc/passwd`
  if [ $USER_EXISTS != "X" ];  then
    /usr/sbin/userdel $TEST_USER
    rm -rf /home/$TEST_USER
  fi

  # pre-crypted password produced with crypt("eal", 42);
  /usr/sbin/useradd -p 42VmxaOByKwlA -m -g nobody $TEST_USER #&> /dev/null

  if [ $? != 0 ]; then
    echo "ERROR: Could not create test user $TEST_USER."
    exit 1
  fi

  AUDIT_STATUS=X`/etc/init.d/auditd status | grep running`
  if [ "$AUDIT_STATUS" = "X" ]; then
     # audit is not running?  well, then start it
     /etc/init.d/auditd start
  fi
}

function clear_env {
  rm -rf /home/$TEST_USER
  /usr/sbin/userdel $TEST_USER
}

# main()

setup_env

ssh01_s1
FIRSTRUN=$?

if [ $FIRSTRUN -eq 0 ]; then
  # the user was able to login when audit was running
  # so turn it off
  /etc/init.d/auditd stop

  # try the test again
  ssh01_s1
  SECONDRUN=$?

  # turn auditing back on
  /etc/init.d/auditd start
fi

clear_env

if [ $SECONDRUN -eq 1 ]; then
  echo TEST_PASSED
  exit 0
else if [ $SECONDRUN -eq 0 ]; then
    echo TEST_FAILED
  else
    echo TEST_ERROR
  fi
fi

exit 1
