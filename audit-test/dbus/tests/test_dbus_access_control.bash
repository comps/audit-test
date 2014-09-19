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
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
#
# DESCRIPTION:
# Tests for D-Bus access control. The tests use dbus-test-server from utils/bin
# which exposes two methods (Test, Exit) on the com.redhat.cc interface.
# The tests configure the service configuration file for dbus-test-server and
# verify that connecting via dbus-send to one of the exposed methods ends as
# expected.
#
# For detailed description of the test scenarios see the test_* functions.
#

source testcase.bash || exit 2
source tp_dbus_functions.bash || exit 2

# be verbose
set -x

#
# Test: default_deny_send_destination
#
# Uses the following policy for testing:
# <policy user="root">
#   <allow own="com.redhat.cctest"/>
# </policy>
#
# Tests if by default the policy is restrictive and denies
# sending message to the test method.
#
test_default_deny_send_destination() {
    # message send should be denied by default
    dbus_msg reject "$DBUS_METHOD_TEST"
}

#
# Test: allow_send_destination
#
# Uses the following policy for testing:
#  <policy user="root">
#    <allow own="com.redhat.cctest"/>
#  </policy>
#  <policy context="default">
#    <allow send_destination="com.redhat.cctest"/>
#  </policy>
#
# Tests if allowing send_destination where the test method is exported
# works.
#
test_allow_send_destination() {
    # add allow send destination in default context
    dbus_add_policy "context=\"default\"" "<allow send_destination=\"${DBUS_IF}\"\/>"

    # message send should be accepted
    dbus_msg accept "$DBUS_METHOD_TEST"
}

#
# Test: deny_send_interface
#
# Uses the following policy for testing:
#  <policy user="root">
#    <allow own="com.redhat.cctest"/>
#  </policy>
#  <policy context="default">
#    <allow send_destination="com.redhat.cctest"/>
#    <deny send_interface="com.redhat.cctest"/>
#  </policy>
#
# Tests if denying send_interface from an allowed destination works.
#
test_deny_send_interface() {
    # add allow send destination in default context
    # policy rules are added on top (i.e. reverse order as with function below)
    dbus_add_policy "context=\"default\"" "<deny send_interface=\"${DBUS_IF}\"\/>"
    dbus_add_policy "context=\"default\"" "<allow send_destination=\"${DBUS_IF}\"\/>"

    # message send should be accepted
    dbus_msg reject "$DBUS_METHOD_TEST"
}

#
# Test: allow_send_interface
#
# Uses the following policy for testing:
# <policy user="root">
#   <allow own="com.redhat.cctest"/>
# </policy>
# <policy context="default">
#   <allow send_interface="com.redhat.cctest"/>
# </policy>
#
# Tests if allowing an send_interface works.
#
test_allow_send_interface() {
    # add allow send destination in default context
    dbus_add_policy "context=\"default\"" "<allow send_interface=\"${DBUS_IF}\"\/>"

    # message send should be accepted
    dbus_msg accept "$DBUS_METHOD_TEST"
}

#
# Test: deny_send_member
#
# Uses the following policy for testing:
#  <policy user="root">
#    <allow own="com.redhat.cctest"/>
#  </policy>
#  <policy context="default">
#    <allow send_interface="com.redhat.cctest"/>
#    <deny send_interface="com.redhat.cctest" send_member="Test"/>
#  </policy>
#
# Tests if denying send_member to the test method works on an allowed interface.
#
test_deny_send_member() {
    # add allow send destination in default context
    # policy rules are added on top (i.e. reverse order as with function below)
    dbus_add_policy "context=\"default\"" "<deny send_interface=\"${DBUS_IF}\" send_member=\"${DBUS_METHOD_TEST}\"\/>"
    dbus_add_policy "context=\"default\"" "<allow send_interface=\"${DBUS_IF}\"\/>"

    # message to Test method should be rejected
    dbus_msg reject "$DBUS_METHOD_TEST"

    # message to List method should be accepted
    dbus_msg accept "$DBUS_METHOD_EXIT"
}

#
# Test: allow_send_member
#
# Uses the following policy for testing:
#  <policy user="root">
#    <allow own="com.redhat.cctest"/>
#  </policy>
#  <policy context="default">
#    <allow send_interface="com.redhat.cctest" send_member="Test"/>
#  </policy>
#
# Tests if allowing send_member for the test method works. Other not
# allowed method on the same interface is is not accessible.
#
test_allow_send_member() {
    # add allow send destination in default context
    dbus_add_policy "context=\"default\"" "<allow send_interface=\"${DBUS_IF}\" send_member=\"${DBUS_METHOD_TEST}\"\/>"

    # message send should be accepted for the test method
    dbus_msg accept "$DBUS_METHOD_TEST"

    # message send should be rejected for the exit method
    dbus_msg reject "$DBUS_METHOD_EXIT"
}

#
# Test: policy_order
#
# Policies are applied to a connection as follows:
#  - all context="default" policies are applied
#  - all group="connection's user's group" policies are applied
#    in undefined order
#  - all user="connection's auth user" policies are applied
#    in undefined order
#  - all at_console="true" policies are applied
#  - all at_console="false" policies are applied
#  - all context="mandatory" policies are applied
#
# Uses the following policy in scenario 1:
#  <policy user="root">
#    <allow send_interface="com.redhat.cctest" send_member="Test"/>
#    <allow own="com.redhat.cctest"/>
#  </policy>
#  <policy context="default">
#    <deny send_interface="com.redhat.cctest" send_member="Test"/>
#  </policy>
#
# Adds the following policy for scenario 2:
#  <policy context="mandatory">
#    <deny send_interface="com.redhat.cctest" send_member="Test"/>
#  </policy>
#
# Scenario 1 - Test if user policy overrides context="default"
# Scenario 2 - Test if context="mandatory" overrides user policy
#
test_policy_order() {
    # add deny destination send destination in default context
    dbus_add_policy "user=\"root\"" "<allow send_interface=\"${DBUS_IF}\" send_member=\"${DBUS_METHOD_TEST}\"\/>"

    # add allow send destination in default context
    dbus_add_policy "context=\"default\"" "<deny send_interface=\"${DBUS_IF}\" send_member=\"${DBUS_METHOD_TEST}\"\/>"

    # message send should be accepted
    dbus_msg accept "$DBUS_METHOD_TEST"

    # add allow send destination in default context
    dbus_add_policy "context=\"mandatory\"" "<deny send_interface=\"${DBUS_IF}\" send_member=\"${DBUS_METHOD_TEST}\"\/>"

    # message send should be accepted
    dbus_msg reject "$DBUS_METHOD_TEST"
}

# create default configuration
dbus_create_config

# run testing
if [ "$(type -t test_$1)" = "function" ]; then
    eval test_$1
else
    help
fi

exit_pass

# vim: ts=4 sw=4 sts=4 ft=sh et ai:
