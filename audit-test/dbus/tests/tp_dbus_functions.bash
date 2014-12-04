###############################################################################
#   Copyright (c) 2014 Red Hat, Inc. All rights reserved.
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
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
#
# DESCRIPTION:
# Various D-Bus related test helpers
#

source functions.bash || exit 2

# global configuration
DBUS_SYSTEM_CONF="/etc/dbus-1/system.d/com.redhat.cctest.conf"
DBUS_TSERVER="./dbus_test_server"
DBUS_IF="com.redhat.cctest"
DBUS_METHOD_TEST="Test"
DBUS_METHOD_EXIT="Exit"
DBUS_PID=

## this variable is used to add rules to the dbus service XML
DBUS_PLACEHOLDER="<!--PLACEHOLDER-->"

# start dbus test server
dbus_server_start() {
    if [ -z "$1" ]; then
        $DBUS_TSERVER &
    else
        runcon -l $1 $DBUS_TSERVER &
    fi
    DBUS_PID=$!
    sleep 1
    ps -hp $DBUS_PID || exit_error "D-Bus test server did not start"
}

# stop dbus server
dbus_server_stop() {
    [ -n "$DBUS_PID" ] && kill -9 $DBUS_PID
}

# dbus test message send function
# $1 - accept/reject
# $2 - dbus method to use
dbus_msg() {
    [ -z "$2" ] && exit_error "No D-Bus method for $FUNCNAME"

    REPLY=$(dbus-send --print-reply --system --dest=com.redhat.cctest / com.redhat.cctest.$2 2>&1)
    RET=$?

    case $1 in
        accept)
            [ $RET -eq 0 ] || exit_fail "dbus failed unexpectedly, reply:\n$REPLY"
            ;;
        reject)
            [ $RET -eq 0 ] && exit_fail "dbus-send passed unexpectedly"
            echo $REPLY | grep -q "Rejected send message" || \
                exit_fail "Unexpected reply:\n$REPLY"
            ;;
        *)
            exit_error "Unknown command for $FUNCNAME"
        ;;
    esac
}

# add dbus allow/deny element into given policy element (specified by attribute)
# $1 - policy element attribute (e.g. context="default")
# $2 - complete deny/allow element (e.g. <deny user="john"/>)
dbus_add_policy() {
    [ -z "$1" -o -z "$2" ] && exit_error "$FUNCNAME requires 2 args"

    if egrep -q "<policy $1>" $DBUS_SYSTEM_CONF; then
        sed -i "s/\(<policy $1>\)/\1\n    $2/" $DBUS_SYSTEM_CONF
    else
        sed -i "s/$DBUS_PLACEHOLDER/  <policy $1>\n    $2\n  <\/policy>\n$DBUS_PLACEHOLDER/" \
            $DBUS_SYSTEM_CONF
    fi

    echo "Current test daemon config '$DBUS_SYSTEM_CONF':"
    cat $DBUS_SYSTEM_CONF

    # restart the test server
    dbus_server_restart
}

# Create clean configuration file. Only root is allowed to own the connection.
# Currently, the system bus has a default-deny policy for sending method calls
# and owning bus names. Everything else, in particular reply messages, receive
# checks, and signals has a default allow policy.
dbus_create_config() {
    cat > $DBUS_SYSTEM_CONF << EOF
<!DOCTYPE busconfig PUBLIC
 "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <policy user="root">
    <allow own="com.redhat.cctest"/>
  </policy>
$DBUS_PLACEHOLDER
</busconfig>
EOF
    append_cleanup "rm -f $DBUS_SYSTEM_CONF"

    # restart dbus to have the new configuration file read
    dbus_restart

    # create config is usually called only once at the beginning of the test
    # this makes it ideal to also add server stop
    append_cleanup "dbus_server_stop"
}

dbus_restart() {
    # make sure new config is applied
    systemctl reset-failed messagebus
    systemctl restart messagebus
}

dbus_server_restart() {
    # restart the message bus to read new configuration files
    dbus_restart

    # start dbus test server again because messagebus restarted
    dbus_server_start
}
