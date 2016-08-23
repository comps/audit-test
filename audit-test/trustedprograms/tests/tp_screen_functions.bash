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
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
#
# DESCRIPTION:
# Screen helper functions
#

source functions.bash || exit 2

#### globals
SCREENRC="/etc/screenrc"
AUTHSRV_DIR="/usr/local/eal4_testing/audit-test/utils/auth-server"

# Remove sleep calls from given file
# $1 - file
function screen_remove_sleep {
    [ "x$1" = "x" ] && exit_error "No file given for $FUNCNAME"
    [ -f $1 ] || exit_error "$FUNCNAME: No file $1 found"

    sed -i "s/.*sleep [0-9]\+.*//g" $1
}

# Check permissions of global screen configuration
# Check if SCREENDIR set to home directory
function screen_check_global_config {
    # Check if config exists
    [ -f $SCREENRC ] || exit_error "Global screen config not found"

    # Check if config is owned by root
    [ "x$(stat -c '%a' $SCREENRC)" != "x644" ] && \
        exit_fail "Permissions of $SCREENRC are not 644"

    # Check if config is owned by root.root
    [ "x$(stat -c '%U%G'  $SCREENRC)" != "xrootroot" ] && \
        exit_fail "$SCREENRC is not by root"

    # Check if screen dir set to home directory
    [ "x$SCREENDIR" == */.screen ] && \
        exit_error "SCREENDIR env variable is not set to \$HOME/.screen!" \
            "Please export it in /etc/profile"
}

# Set idle lockscreen timeout in screen config file
# $1 - screen configuration
# $2 - timeout
function screen_config_set_lock {
    [ "x$1" = "x" ] && exit_error "No config file given for $FUNCNAME"
    [ -f $1 ] || exit_error "$FUNCNAME: No config file $1 found"
    [ "x$2" = "x" ] && exit_error "No idle lockscreen timeout for $FUNCNAME"

    sed -i "s/.*idle.*lockscreen/idle $2 lockscreen/" $1 || \
       exit_error "Error setting lockscreen timeout to $2 in $1"
}

# Check if screen locked after given period of time
# $1 - user
# $2 - user password
# $3 - time
# $4 - optional parameters from screen
# return - 0 on success, other in case of an error
function screen_check_lock {
    [ "x$1" = "x" ] && exit_error "$FUNCNAME: No user given"
    [ "x$2" = "x" ] && exit_error "$FUNCNAME: No user password given"
    [ "x$3" = "x" ] && exit_error "$FUNCNAME: No lock time specified"

    # expect script
    EXPSCRIPT="/home/$1/screen.exp"

    # create expect script in users home, set timeout to 2s
    # after tested timeout
    cat > $EXPSCRIPT << EOT
set timeout $(($3+2))
spawn screen $4
expect {
    {<$1>} {
        expect {Password} { send -- $2\r }
    }
    default { exit 1 }
}
expect {
    {Password} { exit 2 }
    {$1} { send -- exit\r }
}
expect eof { exit 0 }
exit 3
EOT

    # run the expect script as user
    chown ${1}:$1 $EXPSCRIPT
    /bin/su - -c "expect $EXPSCRIPT" $1
    RET=$?

    # remove the expect script
    rm -f $EXPSCRIPT

    return $RET
}

# Check if screen doesn't unlock using bad password
# $1 - user
# $2 - user password
# $3 - time to wait
# $4 - optional parameters from screen
# return - 0 on success, other in case of an error
function screen_check_badpass {
    [ "x$1" = "x" ] && exit_error "$FUNCNAME: No user given"
    [ "x$2" = "x" ] && exit_error "$FUNCNAME: No user password given"
    [ "x$3" = "x" ] && exit_error "$FUNCNAME: No lock time specified"

    # expect script
    EXPSCRIPT="/home/$1/screen.exp"

    # create expect script in users home, set timeout to 1s
    # after tested timeout
    cat > $EXPSCRIPT << EOT
set timeout $(($3+1))
spawn screen $4
sleep 1
expect {
    {Password} { send -- BADPASS\r }
    default { exit 1 }
}
expect {
    {Password} { send -- $2\r }
    default { exit 2 }
}
expect {
    {$1} { send -- exit\r }
    default { exit 3 }
}
expect eof { exit 0 }
exit 4
EOT

    # run the expect script as user
    chown ${1}:$1 $EXPSCRIPT
    /bin/su - -c "expect $EXPSCRIPT" $1
    RET=$?

    # remove the expect script
    rm -f $EXPSCRIPT

    return $RET
}

# Check if screen clears content using clear(1) escape sequence
# $1 - user
# $2 - user password
# $3 - time to wait
# $4 - optional parameters from screen
# return - 0 on success, other in case of an error
function screen_check_clear {
    [ "x$1" = "x" ] && exit_error "$FUNCNAME: No user given"
    [ "x$2" = "x" ] && exit_error "$FUNCNAME: No user password given"
    [ "x$3" = "x" ] && exit_error "$FUNCNAME: No lock time specified"

    # expect script
    EXPSCRIPT="/home/$1/screen.exp"

    # create expect script in users home, set timeout to 1s
    # after tested timeout check for clear control sequence
    #
    # the test expects the clear sequence for terminals linux,
    # screen.linux and xterm. On other terminal types the check
    # may possibly fail!
    #
    # To check you terminal type use the command
    # echo $TERM
    #
    prepend_cleanup "rm -f $EXPSCRIPT"
    cat > $EXPSCRIPT << EOT
set timeout $(($3+1))
spawn screen $4
expect {$1} {
    expect {Password} { send -- $2\r }
    default { exit 1 }
}
expect {
    {$1} { send -- exit\r }
    default { exit 2 }
}
expect eof { exit 0 }
exit 3
EOT

    # run the expect script as user
    chown ${1}:$1 $EXPSCRIPT
    EXPOUT=$(mktemp)
    prepend_cleanup "rm -f $EXPOUT"
    /bin/su - -c "expect $EXPSCRIPT" $1 &> $EXPOUT
    hexdump -C $EXPOUT
    # count the number of lines with clear screen
    # screen should clear the screen 2 times after running
    # and once after locking the screen
    CLRCNT=$(cat $EXPOUT | xxd -p | tr -d '\n' | grep -o "$(clear | xxd -p)" | wc -l)
    [ "$CLRCNT" -eq 3 ] && RET=0 || RET=1

    # check if kernel cmdline contains required options for
    # disabling framebuffer scrolling
    grep "no-scroll" /proc/cmdline || RET=2
    grep "fbcon=scrollback:0" /proc/cmdline || RET=3

    return $RET
}

# Check if
# + screen locking via "CTRL-A x" works
# + control sequences (CTRL-C, CTRL-D, CTRL-A d)
#   do not work when screen locked
# $1 - user
# $2 - user password
# return - 0 on success, other in case of an error
function screen_check_sequences {
    [ "x$1" = "x" ] && exit_error "$FUNCNAME: No user given"
    [ "x$2" = "x" ] && exit_error "$FUNCNAME: No user password given"

    # expect script
    EXPSCRIPT="/home/$1/screen.exp"

    # create expect script in users home
    # check if
    # + screen locking via "CTRL-A x" works
    # + control sequences (CTRL-C, CTRL-D, CTRL-A d)
    #   do not work when screen locked
    cat > $EXPSCRIPT << EOT
set timeout 5
spawn screen
sleep 1
expect {
    {$1} { send -- \\x01x }
    default { exit 1 }
}
expect {
    {Password} { send -- \\x03 }
    default { exit 2 }
}
expect {
    {$1} { exit 3 }
    default { send -- \\x04 }
}
expect {
    {$1} { exit 4 }
    default { send -- \\x01d }
}
expect {
    {$1} { exit 5 }
    default { send -- ANYPASS\r }
}
expect {
    {Password} { send -- $2\r }
    default { exit 6 }
}
expect {
    {$1} { send -- exit\r }
    default { exit 7 }
}
expect eof { exit 0 }
exit 8
EOT

    # run the expect script as user
    chown ${1}:$1 $EXPSCRIPT
    /bin/su - -c "expect $EXPSCRIPT" $1
    RET=$?

    # remove the expect script
    rm -f $EXPSCRIPT

    return $RET
}
