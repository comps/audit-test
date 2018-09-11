#!/bin/bash -x
###############################################################################
#   Copyright (c) 2014 Red Hat, Inc. All rights reserved.
#
#   This copyrighted material is made available to anyone wishing
#   to use, modify, copy, or redistribute it subject to the terms
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
# User and group account lifecycle
#
# AUTHOR: Ondrej Moris <omoris@redhat.com>
#
# DESCRIPTION: This test verifies that event during user and group account
# lifecycle are generated correctly according to the specification [1].
#
# [1] http://people.redhat.com/sgrubb/audit/user-account-lifecycle.txt

source tp_auth_functions.bash || exit 2

unset user
unset uid
unset group
unset gid

# Check if an unique event of given type with given message was generated.
#
# $1 - expectec count of events
# $2 - message type
# $3 - message content
#
# Return 0 if such event if found, 1 if not, 2 if the event is not unique,
# 3 if it does not contain desired message, 3 otherwise.
#
# Exit with error if parameters are missing.
#
function check_event {

    local count=$1
    local type=$2
    local message=$3

    [ -z "$count"   ] && exit_error "Missing count parameter!"
    [ -z "$type"    ] && exit_error "Missing type parameter!"
    [ -z "$message" ] && exit_error "Missing message parameter!"

    local result=3

    # Wait for all events to populate (until the default timeout).
    wait_for_cmd "[ $(ausearch -m $type -ts $date_mark --raw | wc -l) -ge $count ]"

    # Store events for subsequent verification.
    local event=$(mktemp)
    ausearch -m $type -ts $date_mark --raw > $event

    # Verify event contents and possible uniqueness.
    if [ $? -eq 0 ]; then
        if [ $(cat $event | wc -l) -eq $count ]; then
            if ! egrep -q "msg='$message.*'" $event; then
                echo "Event does not contain correct message"
                cat $event
                result=2
            else
                echo "Success, correct event(s) found"
                cat $event
                result=0
            fi
        else
            echo "Unexpected count of events"
            cat $event
            result=2
        fi
    else
        echo "No event found"
        result=1
    fi

    rm -f $event

    return $result
}

# Set user or group password.
#
# $1 - user/group
# $2 - subject name (username or groupname)
# $3 - new password
#
# Return 0 if new password was set correctly, non-zero otherwise.
#
# Exit with error if parameters are missing.
#
function set_password {

    sleep 2
    date_mark="$(date +%T)"

    local subject=$2
    local password=$3

    [ -z "$subject"  ] && exit_error "Missing subject parameter!"
    [ -z "$password" ] && exit_error "Missing password parameter!"

    if [ "$1" == "user" ]; then
        local tool="passwd"
    elif [ "$1" == "group" ]; then
        local tool="gpasswd"
    else
        exit_error "Missing user/group parameter!"
    fi

    expect -c "
      spawn $tool $subject
      expect -nocase -re {new password:} {
        send \"$password\r\"
        expect -nocase -re {new password:} {
          send \"$password\r\"
          exp_wait
          exit 0
        }
      }
      exit 1"

    return $?
}

# Execute command and wait for its termination, 2 seconds delay before
# execution if enforced to make sure that date_mark will be bigger than
# the previous one (date_mark is used in check_event function).
#
# $1 - command
#
# Return exit code of executed command.
#
function setpid {
    sleep 2
    date_mark="$(date +%T)"
    "$@" &
    pid=$!
    wait $pid
    return $?
}


# Main test
# =========
#
# The following user/group account lifecycle is simulated:
#
# 1. User X is created, one ADD_USER. With MLS selinux policy there
#    is also one ASSIGN_ROLE. In MLS, we also remove assigned role
#    to see multiple ROLE_REMOTE events.

[ "$PPROFILE" == "lspp" ] && seuser="-Z staff_u"

read user uid <<<"$(generate_unique_user)"
setpid useradd $seuser -u $uid $user || exit_error "useradd failed"
prepend_cleanup "killall -9 -u '$user'; groupdel '$user'; userdel -rf '$user'"

check_event 1 "ADD_USER" "op=add-user id=$uid" || \
    exit_fail "ADD_USER was not generated correctly"

if [ "$PPROFILE" == "lspp" ]; then
    check_event 1 "ROLE_ASSIGN" "op=login-sename,role,range acct=\"$user\"" || \
        exit_fail "ROLE_ASSIGN was not generated correctly"

    setpid semanage login -d $user

    check_event 2 "ROLE_REMOVE" "op=login acct=\"$user\"" || \
        exit_fail "ROLE_REMOVE was not generated correctly"
fi

# 2. Group Y is created, one ADD_GROUP event and one GRP_MGMT
#    evens are generated.

read group gid <<<"$(generate_unique_group)"
setpid groupadd -g $gid $group || exit_error "groupadd failed"
prepend_cleanup "groupdel '$group'"

check_event 1 "ADD_GROUP" "op=add-group id=$gid" || \
    exit_fail "ADD_GROUP was not generated correctly"
check_event 1 "GRP_MGMT" "op=add-shadow-group id=$gid" || \
    exit_fail "ADD_GROUP was not generated correctly"

# 3. User X is added to users and Y groups, which should produce one
#    USER_MGMT event, user X group assignment is then reverted
#    by removing user X from group Y. It should produce another GRP_MGMT.

setpid usermod -G users,$group $user || exit_error "usermod failed"

check_event 4 "USER_MGMT" \
    "op=add-user-to-group grp=\"users\" acct=\"$user\"" || \
    exit_fail "USER_MGMT was not generated correctly"
check_event 4 "USER_MGMT" \
    "op=add-user-to-group grp=\"$group\" acct=\"$user\"" || \
    exit_fail "USER_MGMT was not generated correctly"
check_event 4 "USER_MGMT" \
    "op=add-user-to-shadow-group grp=\"users\" acct=\"$user\"" || \
    exit_fail "USER_MGMT was not generated correctly"
check_event 4 "USER_MGMT" \
    "op=add-user-to-shadow-group grp=\"$group\" acct=\"$user\"" || \
    exit_fail "USER_MGMT was not generated correctly"

setpid usermod -G users $user || exit_error "usermod failed"

check_event 2 "USER_MGMT" \
    "op=delete-user-from-group grp=\"$group\" acct=\"$user\"" || \
    exit_fail "USER_MGMT was not generated correctly"
check_event 2 "USER_MGMT" \
    "op=delete-user-from-shadow-group grp=\"$group\" acct=\"$user\"" || \
    exit_fail "USER_MGMT was not generated correctly"

# 4. Then we modify various attributes of user X in the following
#    order: comment, expiration date, primary group, home directory,
#    inactive days, login name and login shell. All except primary
#    group and login name changes should produce a single
#    USER_MGMT event while primary group and login name changes
#    should produce multiple USER_MGMT events.

setpid usermod -c "test" $user || exit_error "usermod failed"

check_event 1 "USER_MGMT" "op=changing-comment id=$uid" || \
    exit_fail "USER_MGMT was not generated correctly"

setpid usermod -e 20 $user || exit_error "usermod failed"

check_event 1 "USER_MGMT" "op=changing-expiration-date id=$uid" || \
    exit_fail "USER_MGMT was not generated correctly"

setpid usermod -g $gid $user || exit_error "usermod failed"
setpid usermod -g $uid $user || exit_error "usermod failed"

check_event 2 "USER_MGMT" "op=changing-primary-group id=$uid" || \
    exit_fail "USER_MGMT was not generated correctly"

setpid usermod -d /home/${user}X  $user || exit_error "usermod failed"
setpid usermod -d /home/$user  $user || exit_error "usermod failed"

check_event 1 "USER_MGMT" "op=changing-home-dir id=$uid" || \
    exit_fail "USER_MGMT was not generated correctly"

setpid usermod -f 10 $user || exit_error "usermod failed"

check_event 1 "USER_MGMT" "op=changing-inactive-days id=$uid" || \
    exit_fail "USER_MGMT was not generated correctly"

setpid usermod -l ${user}X $user || exit_error "usermod failed"
setpid usermod -l $user ${user}X || exit_error "usermod failed"

check_event 4 "USER_MGMT" "op=changing-name id=$uid" || \
    exit_fail "USER_MGMT was not generated correctly"

setpid usermod -s /bin/true $user || exit_error "usermod failed"

check_event 1 "USER_MGMT" "op=changing-shell id=$uid" || \
    exit_fail "USER_MGMT was not generated correctly"


# 5. Finally, we change password of user X. First, via usermod command
#    and then via passwd. First action should generate a single
#    USER_CHAUTHTOK event, second should generate multiple
#    such events.

setpid usermod -p "$PASSWD" $user || exit_error "usermod failed"

check_event 1 "USER_CHAUTHTOK" "op=updating-password id=$uid" || \
    exit_fail "USER_CHAUTHTOK was not generated correctly"

set_password "user" $user $PASSWD

check_event 2 "USER_CHAUTHTOK" "op=change password id=$uid" || \
    exit_fail "USER_CHAUTHTOK was not generated correctly"
check_event 2 "USER_CHAUTHTOK" \
    "op=PAM:chauthtok grantors=pam_pwquality,pam_unix acct=\"$user\"" || \
    exit_fail "USER_CHAUTHTOK was not generated correctly"

# 6. At this point we do not need user X anymore and its account
#    is deleted, it should generate a single DEL_USER event.

setpid userdel $user || exit_error "userdel failed"

check_event 1 "DEL_USER" "op=delete-user id=$uid" || \
    exit_fail "DEL_USER was not generated correctly"

# 7. Now we just change group ID and then revert it back, operation
#    should produce three GRP_MGMT events distunguished by different
#    audit messages.

setpid groupmod -g $[$gid+10] $group || exit_error "groupmod failed"
setpid groupmod -g $gid $group || exit_error "groupmod failed"

check_event 3 "GRP_MGMT" "op=changing-group grp=\"$group\"" || \
    exit_fail "GRP_MGMT was not generated correctly"
check_event 3 "GRP_MGMT" "op=changing-group-passwd grp=\"$group\"" || \
    exit_fail "GRP_MGMT was not generated correctly"
check_event 3 "GRP_MGMT" "op=modify-group acct=\"$group\"" || \
    exit_fail "GRP_MGMT was not generated correctly"

# 8. Then we first change group password and then we remove the password
#    completely, both actions should gnerate single GRP_CHAUTHTOK
#    events.

set_password "group" $group $PASSWD

check_event 1 "GRP_CHAUTHTOK" "op=change-password grp=\"$group\"" || \
    exit_fail "GRP_CHAUTHTOK was not generated correctly"

setpid gpasswd -r $group

check_event 1 "GRP_CHAUTHTOK" "op=delete-group-password grp=\"$group\"" || \
    exit_fail "GRP_CHAUTHTOK was not generated correctly"

# 9. At this point, we do not need group Y anymore and we delete it which
#    generate a single DEL_GROUP and a single GRP_MGMT event.

setpid groupdel $group || exit_error "groupdel failed"
check_event 1 "DEL_GROUP" "op=delete-group id=$gid" || \
    exit_fail "DEL_GROUP was not generated correctly"
check_event 1 "GRP_MGMT" "op=delete-shadow-group id=$gid" || \
    exit_fail "DEL_GROUP was not generated correctly"

exit_pass
