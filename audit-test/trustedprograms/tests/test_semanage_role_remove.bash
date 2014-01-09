#!/bin/bash
###############################################################################
#   Copyright (c) 2011 Red Hat, Inc. All rights reserved.
#
#   This copyrighted material is made available to anyone wishing
#   to use, modify, copy, or redistribute it /bin/subject to the terms
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
# PURPOSE:
# Use semanage to remove user SELinux login record. Check if the removal was
# successful and it gets audited with ROLE_REMOVE event.
#

source tp_auth_functions.bash || exit 2

# the added users range is taken from the default range
# of the user who is running semanage:
# see https://bugzilla.redhat.com/show_bug.cgi?id=785678#c8
def_range=s0-s15:c0.c1023
def_context=staff_u:staff_r:staff_t:$def_range

# add new user with default role
auid=$(cat /proc/self/loginuid)
seuser="staff_u"
useradd -Z $seuser -n -u $uid $user || exit_error "useradd failed"

# remove the SELinux login record for the user
semanage login -d $user

# verify if semanage successful
if [ $? -ne 0 ]; then
  exit_error "semange returned an error while removing SELinux login"
fi

# verify user removed
semanage login -l | grep $user
if [ $? -eq 0 ]; then
  exit_fail "semange login -l still shows SELinux login record"
fi

# find out the default roles for $seuser role
role=$(semanage user -l | awk "/$seuser/ {for(i=5; i<NF; i++) { printf \"%s,\", \$i } printf \"%s\", \$NF}")
[ -z "$role" ] && exit_error "Cannot determine $seuser roles"

# check for correct ROLE_ASSIGN audit record
msg_1="op=login-sename,role,range acct=\"$user\" old-seuser=\? old-role=\? old-range=\? new-seuser=staff_u new-role=$role new-range=$def_range exe=\"/usr/sbin/useradd\".*res=success.*"

augrok -q type=ROLE_ASSIGN auid=$auid msg_1=~"$msg_1" \
	|| exit_fail "ROLE_ASSIGN event missing: \"$msg_1\""

# check for ROLE_REMOVE audit record
msg_1="op=login acct=\"$user\" old-seuser=$seuser old-role=$role old-range=$def_range new-seuser=user_u new-role=user_r new-range=s0 exe=/usr/sbin/semanage.*res=success.*"

augrok -q type=ROLE_REMOVE auid=$auid msg_1=~"$msg_1" \
	|| exit_fail "ROLE_REMOVE event missing: \"$msg_1\""

# cleanup
# deluser handled by tp_auth_functions

exit_pass
