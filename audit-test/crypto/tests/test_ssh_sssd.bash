#!/bin/bash
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
# FIA_UAU.1(RITE), FIA_UAU.5
#
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
#
# DESCRIPTION:
# This test implements ssh related sssd testcases.
#
# Testcases:
#  login - tests simple pass/fail scenario for correct/incorrect credentials
#  expired - test if expired password causes asking for change during login
#  selinux - test if logged in user gets correct SELinux context
#

source testcase.bash || exit 2
source tp_ssh_functions.bash || exit 2

# globals
AUTHSRV_DIR="/usr/local/eal4_testing/audit-test/utils/auth-server"

# be verbose
set -x

###############################################################################
## Setup phase
###############################################################################

function ssh_sssd_setup {
    local USER=

    # source ipa defaults
    source $AUTHSRV_DIR/ipa_env || \
        exit_error "Could not source ipa defaults from $AUTHSRV_DIR/ipa_env"

    # check for required default from ipa_env
    for EVAR in IPA_PASS IPA_USER IPA_STAFF IPA_USER_EXPIRED; do
        [ -z "$(eval echo \$$EVAR)" ] && \
            exit_error "Required $EVAR not found in environment"
    done

    # backup global profile and remove sleep
    disable_ssh_strong_rng

    for USER in $IPA_USER $IPA_STAFF $IPA_USER_EXPIRED; do
        # clear faillock at cleanup
        prepend_cleanup "faillock --reset --user $USER"
        # make sure mappings removed after testing
        prepend_cleanup "rm -f /etc/selinux/mls/logins/$USER"
    done

    # enable sssd
    restart_service sssd
    prepend_cleanup "stop_service sssd"

}

###############################################################################
## FIA_UAU.1(RITE)
###############################################################################

function login {
    local RET=

    # positive test - sssd auth with correct password
    ssh_connect_pass $IPA_USER $IPA_PASS $IPA_STAFF $IPA_PASS || \
        exit_fail "Failed to connect ifrom $IPA_USER to $IPA_STAFF with correct password"

    # negative test - sssd auth with invalid password
    ssh_connect_pass $IPA_USER $IPA_PASS $IPA_STAFF BADPASS; RET=$?
    [ $RET -eq 0 ] && exit_fail "Connection passed with invalid password"
    [ $RET -eq 2 ] || exit_fail "Connection failed unexpectedly ($RET)"
}

###############################################################################
## FIA_UAU.5
###############################################################################

function expired {
    local RET=

    ssh_connect_pass $IPA_USER $IPA_PASS $IPA_USER_EXPIRED $IPA_PASS; RET=$?
    [ $RET -eq 0 ] && exit_fail "Succeeded to connect with expired password"
    [ $RET -eq 4 ] || exit_error "Connection failed unexpectedly. No expire info?"
}

###############################################################################
## SELinux
###############################################################################

function selinux {
    local OUT= IDZ= CONTEXT=

    # user_u
    CONTEXT="user_u:user_r:user_t:SystemLow"
    OUT=$(ssh_cmd $IPA_USER $IPA_PASS "id -Z"); RET=$?
    [ $RET -eq 0 ] || exit_error "Error connecting as $IPA_USER"
    echo -n "ID: "
    IDZ=$(echo $OUT | egrep -o '[[:alnum:]_]+:[[:alnum:]_]+:[[:alnum:]_]+:[[:alnum:]:\.-]*')
    [ "$CONTEXT" = "$IDZ" ] || \
        exit_fail "$IDZ does not match expected $CONTEXT"

    # staff_u
    if [ "$PPROFILE" == "lspp" ]; then
        CONTEXT="staff_u:staff_r:staff_t:SystemLow-s0:c0.c1023"
    else
        CONTEXT="staff_u:staff_r:staff_t:SystemLow-SystemHigh"
    fi
    OUT=$(ssh_cmd $IPA_STAFF $IPA_PASS "id -Z"); RET=$?
    echo $OUT
    [ $RET -eq 0 ] || exit_error "Error connecting as $IPA_USER"
    IDZ=$(echo $OUT | egrep -o '[[:alnum:]_]+:[[:alnum:]_]+:[[:alnum:]_]+:[[:alnum:]:\.-]*')
    [ "$CONTEXT" = "$IDZ" ] || \
        exit_fail "$IDZ does not match expected $CONTEXT"

}

ssh_sssd_setup

# choose test
if [ "$(type -t $1)" = "function" ]; then
    eval $1
else
    exit_error "Uknown test or no test given"
fi

exit_pass

# vim: ts=4 sw=4 sts=4 ft=sh et ai:
