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
# SSH helper functions
#

source functions.bash || exit 2

#### global configuration
# default backup folder, needs to be writable by the user!
BFOLDER="/tmp"
RND=$RANDOM
# default expect timeout 10 minutes - in case of not enough entropy
TIMEOUT=600

# Remove SSH_USE_STRONG_RNG from environment
function ssh_remove_strong_rng_env {
    export -n SSH_USE_STRONG_RNG
    prepend_cleanup "export SSH_USE_STRONG_RNG=12; readonly SSH_USE_STRONG_RNG"
}

# Remove SSH_USE_STRONG_RNG exporting from give file
# Does nothing if given file does not exist
# $1 - file
function ssh_remove_strong_rng {
    [ "x$1" = "x" ] && exit_error "No file given for $FUNCNAME"
    [ -f $1 ] || return

    sed -i "s/.*SSH_USE_STRONG_RNG.*//g" $1
}

# Remove sleep calls from given file
# Does nothing if given file does not exist
# $1 - file
function ssh_remove_screen {
    [ "x$1" = "x" ] && exit_error "No file given for $FUNCNAME"
    [ -f $1 ] || return

    sed -i "s/.*sleep [0-9]\+.*//g; s/.*exec.*SCREENEXEC.*//g" $1
}

# Backup the .ssh directory of the specified user. The backup
# is created in $BFOLDER folder under 'user_$RND_sshbackup.tgz'.
# $1 - user
function ssh_backup_home {
    # check if required user specified
    [ "x$1" = "x" ] && exit_error "Error: no user given for $FUNCNAME"

    # if nothing to backup bail out
    /bin/su - -c "test -d .ssh" $1 || return

    # creating the backup is ran under the user because getting home folder
    # in MLS can be tricky
    BFILE="$BFOLDER/$1_${RND}_sshbackup.tgz"
    /bin/su - -c "tar --selinux -czpv -f $BFILE .ssh" $1 || \
        exit_error "Error creating .ssh backup $BFILE"
}

# Wipe the .ssh directory of the specified user.
# $1 - user
# $2 - password
function ssh_cleanup_home {
    # check if required parameters specified
    [ "x$1" = "x" ] && exit_error "Error: no user given for $FUNCNAME"
    [ "x$2" = "x" ] && exit_error "Error: no password given for $FUNCNAME"

    ssh_cmd $1 $2 "rm -rf .ssh"
}

# Restore the .ssh directory of the specified user.
# If .ssh folder exitsts wipe it before the restore.
# $1 - user
function ssh_restore_home {
    # check if required user specified
    [ "x$1" = "x" ] && exit_error "Error: no user given for $FUNCNAME"

    # wipe the .ssh folder
    /bin/su - -c "rm -rf .ssh" $1

    BFILE="$BFOLDER/$1_${RND}_sshbackup.tgz"

    # if nothing to restore
    /bin/su - -c "test -f $BFILE" $1 || return

    # restore the backup
    /bin/su - -c "tar xvf $BFILE" $1 || \
        exit_error "Error restoring .ssh backup from $BFILE"

    # remove the backup
    rm -f $BFILE
}

# Create auth key for ssh with no password in default location
# $1 - user
# $2 - user password
# $3 - type (rsa/dsa)
# $4 - number of bits in the key
# $5 - passphrase for key (optional)
function ssh_create_key {
    # check if required user specified
    [ "x$1" = "x" ] && exit_error "Error: no user given for $FUNCNAME"
    [ "x$2" = "x" ] && exit_error "Error: no password given for $FUNCNAME"
    [ "x$3" = "x" ] && exit_error "Error: no key type given for $FUNCNAME"
    [ "x$4" = "x" ] && exit_error "Error: no key size given for $FUNCNAME"

    # generate the keys
    expect -c "set timeout $TIMEOUT
        spawn ssh $1@localhost \"ssh-keygen -t $3 -b $4 -N '$5' -f \\\$(pwd)/.ssh/id_$3\"
        expect {
            {yes/no} { send -- \"yes\r\"; exp_continue }
            {Password} { send -- \"$2\r\" }
            default { exit 1 }
        }
        expect {randomart} { exit 0 }
        exit 2"

    [ $? -ne 0 ] && exit_fail "Cannot create $3 key"
}

# Check ssh key for correct cipher and key length using openssl
# $1 - user
# $2 - user password
# $4 - number of bits in the key
function ssh_check_key {
    # check if required user specified
    [ "x$1" = "x" ] && exit_error "Error: no user given for $FUNCNAME"
    [ "x$2" = "x" ] && exit_error "Error: no password given for $FUNCNAME"
    [ "x$3" = "x" ] && exit_error "Error: no key type given for $FUNCNAME"
    [ "x$4" = "x" ] && exit_error "Error: no key size given for $FUNCNAME"

    # check key
    ssh_cmd $1 $2 "openssl $3 -in /home/$1/.ssh/id_$3 -text" | grep "$4 bit" || \
        exit_fail "Key /home/$1/.ssh/id_$3 has incorrect size"
}


# Run command via ssh.
# $1 - user
# $2 - password
# $3 - cmd
function ssh_cmd {
    # check if required params specified
    [ "x$1" = "x" ] && exit_error "Error: no user for $FUNCNAME"
    [ "x$2" = "x" ] && exit_error "Error: no password for $FUNCNAME"
    [ "x$3" = "x" ] && exit_error "Error: no comamnd for $FUNCNAME"

    expect -c "set timeout $TIMEOUT
        spawn ssh $1@localhost \"$3\"
        expect {
            {yes/no} { send -- yes\r; exp_continue }
            {Password} { send -- $2\r }
        }
        expect eof { exit 0 }
        exit 1"

    [ $? -ne 0 ] && exit_fail "Failed to run $3 as $1"
}

# Run command via ssh as root in mls mode.
# $1 - cmd
function ssh_mls_cmd {

    # check if required params specified
    [ "x$1" = "x" ] && exit_error "Error: no comamnd for $FUNCNAME"

    local M="${RANDOM}${RANDOM}${RANDOM}"

    expect -c "set timeout $TIMEOUT
      spawn ssh eal@localhost
      expect {
        {yes/no} { send -- yes\r; exp_continue }
        {assword} { send -- $PASSWD\r }
        default { exit 1 }
      }
      expect {
        {get default type} { exit 1 }
        {eal} { send -- \"newrole -r lspp_test_r\n\"; exp_continue }
        {assword} { send -- \"$PASSWD\r\" }
        default { exit 1 }
      }
      expect {
        {eal} { send -- \"/bin/su -\r\"; exp_continue }
        {assword} { send -- \"$PASSWD\r\" }
        default { exit 1 }
      }
      expect {root} { send -- \"$1 && (echo -n $M && echo PASS) || (echo -n &M && echo FAIL)\r\" }
      expect {
        {${M}PASS} { exit 0 }
        {${M}FAIL} { exit 1 }
      }
      exit 2"

    [ $? -ne 0 ] && exit_fail "Failed to run $1 as root in mls mode" && return 1

    return 0
}

# Check permissions and selinux context of the .ssh directory and the
# underlying public and private key
# $1 - user
# $2 - password
# $3 - type (rsa/dsa)
function ssh_check_home {
    # check if required params specified
    [ "x$1" = "x" ] && exit_error "Error: no user for $FUNCNAME"
    [ "x$2" = "x" ] && exit_error "Error: no password for $FUNCNAME"
    [ "x$3" = "x" ] && exit_error "Error: no key type for $FUNCNAME"

    # check permissions
    ssh_cmd $1 $2 "stat -c '%a' /home/$1/.ssh" | grep 700 || \
        exit_fail "Folder .ssh has bad permissions"
    ssh_cmd $1 $2 "stat -c '%a' /home/$1/.ssh/id_$3" | grep 600 || \
        exit_fail "File id_$3 has bad permissions"
    ssh_cmd $1 $2 "stat -c '%a' /home/$1/.ssh/id_$3.pub" | grep 640 || \
        exit_fail "File id_$3.pub has bad permissions"

    # check SELinux permissions
    ssh_cmd $1 $2 "stat -c '%C' /home/$1/.ssh" | grep ssh_home_t || \
        exit_fail "Folder .ssh has bad SELinux context"
    ssh_cmd $1 $2 "stat -c '%C' /home/$1/.ssh/id_$3" | grep ssh_home_t || \
        exit_fail "File id_$3 has bad SELinux context"
    ssh_cmd $1 $2 "stat -c '%C' /home/$1/.ssh/id_$3.pub" | grep ssh_home_t || \
        exit_fail "File id_$3.pub has bad SELinux context"
}

# Copy key from user1 to user2 using ssh-copy-id and test if successful
# $1 - source user
# $2 - source user password
# $3 - destination user
# $4 - destination user password
# $5 - rsa/dsa - specifies key to copy from .ssh folder
function ssh_copy_key {
    # check if required params specified
    [ "x$1" = "x" ] && exit_error "Error: no src user for $FUNCNAME"
    [ "x$2" = "x" ] && exit_error "Error: no src user password for $FUNCNAME"
    [ "x$3" = "x" ] && exit_error "Error: no dst user for $FUNCNAME"
    [ "x$4" = "x" ] && exit_error "Error: no dst user password for $FUNCNAME"
    [ "x$5" = "x" ] && exit_error "Error: no key for $FUNCNAME"

    # copy key from user1 to user2
    PRIVKEY="/home/$1/.ssh/id_$5"
    expect -c "set timeout $TIMEOUT
        spawn ssh $1@localhost
        expect {
            {yes/no} { send -- yes\r; exp_continue }
            {Password} { send -- $2\r }
        }
        expect {$1}
        send -- \"ssh-copy-id -i $PRIVKEY $3@localhost\r\"
        expect {
            {yes/no} { send -- yes\r; exp_continue }
            {Password} { send -- $4\r }
            default { exit 1 }
        }
        expect {try logging}
        send -- \"exit\r\"
        expect eof { exit 0 }
        exit 2"

    [ $? -ne 0 ] && exit_fail "Failed to copy $PRIVKEY from $1 to $3"
}

# Connect from user1 to user2 without password and test if successful
# $1 - source user
# $2 - source user password
# $3 - destination user
# $4 - key passphrase (optional)
function ssh_connect_nopass {
    # check if required params specified
    [ "x$1" = "x" ] && exit_error "Error: no src user for $FUNCNAME"
    [ "x$2" = "x" ] && exit_error "Error: no src user password for $FUNCNAME"
    [ "x$3" = "x" ] && exit_error "Error: no dst user for $FUNCNAME"

    # connect from user $1 to user $3
    expect -c "set timeout $TIMEOUT
        spawn ssh $1@localhost
        expect {
            {yes/no} { send -- yes\r; exp_continue }
            {Password} { send -- $2\r }
        }
        expect {$1}
        send -- \"ssh $3@localhost whoami\r\"
        expect {
            {yes/no} { exit 1 }
            {$3} { exit 0 }
            {Password} { exit 2 }
            {passphrase} { send -- $4; exp_continue }
            default { exit 3 }
        }
        exit 4"

    [ $? -ne 0 ] && exit_fail "$FUNCNAME: Failed to connect from $1 to $3"
}

# Connect from user1 to user2 with password and test if successful
# $1 - source user
# $2 - source user password
# $3 - destination user
# $4 - destination user password
# $5 - optional string passed to ssh
# return value - 0 on success / other fail
function ssh_connect_pass {
    # check if required params specified
    [ "x$1" = "x" ] && exit_error "Error: no src user for $FUNCNAME"
    [ "x$2" = "x" ] && exit_error "Error: no src user password for $FUNCNAME"
    [ "x$3" = "x" ] && exit_error "Error: no dst user for $FUNCNAME"
    [ "x$4" = "x" ] && exit_error "Error: no dst user password for $FUNCNAME"

    # connect from user $1 to user $3
    expect -c "set timeout $TIMEOUT
        spawn ssh $1@localhost
        expect {
            {yes/no} { send -- yes\r; exp_continue }
            {Password} { send -- $2\r }
        }
        expect {$1}
        send -- \"ssh $5 $3@localhost whoami\r\"
        expect {
            {yes/no} { send -- yes\r; exp_continue }
            {Password} { send -- $4\r }
            {passphrase} { send -- INCORRECTPASS\r; exp_continue }
            default { exit 1 }
        }
        expect {
            {Password} { send -- $4\r; exp_continue }
            {Permission denied} { exit 2 }
            {$3} { exit 0 }
        }
        exit 3"

    # return expect return value
    return $?
}

# Check audit records for ssh connection. Parameters $2 and $3 are optional.
# Please note that to use the paramater $3 you need to supply also $2
# $1 - audit mark
# $2 - pass|fail - check for successdul/failed login attempt (default pass)
# $3 - cipher type (default any)
function ssh_check_audit {
    # check if required params specified
    [ "x$1" = "x" ] && exit_error "Error: no log mark given for $FUNCNAME"
    [ "x$2" = "xfail" ] && FAIL=yes
    [ "x$3" != "x" ] && GROKCIPHER="msg_1=~$3"

    if [ "x$FAIL" = "xyes" ]; then
        # USER_ERR audit record at unsuccessful login
        augrok --seek=$1 type==USER_ERR \
            || exit_fail "Expected audit record USER_ERR not found."
    else
        # Check for required audit messages
        augrok --seek=$1 type=CRYPTO_SESSION $GROKCIPHER || exit_fail \
            "Expected audit record CRYPTO_SESSION $3 not found."

        for TYPE in CRYPTO_KEY_USER CRED_ACQ LOGIN; do
            augrok --seek=$1 type==$TYPE \
                || exit_fail "Expected audit record $TYPE not found."
        done
        # no USER_ERR audit record at successful login
        augrok --seek=$1 type==USER_ERR \
            && exit_fail "Not expected audit record USER_ERR found."
    fi
}

# This function removes the strong rng configuration for ssh
# and causes it to be restored at the end of the test.  This
# should only be used for test cases that are using ssh as a
# test tool and not explicitly testing the rng properties of ssh.
# The function also disables the sleep in /etc/profile after
# login to speed up the testing.
function disable_ssh_strong_rng {
    MPROFILE="/etc/profile"
    SSHDCONF="/etc/sysconfig/sshd"
    CCCONF="/etc/profile.d/cc-configuration.sh"

    # backup global profile and remove sleep
    [ -e $MPROFILE ] && backup $MPROFILE
    [ -e $SSHDCONF ] && backup $SSHDCONF
    [ -e $CCCONF ] && backup $CCCONF

    # remove screen from /etc/profile
    ssh_remove_screen $MPROFILE

    # remove SSH_USE_STRONG_RNG from environment
    ssh_remove_strong_rng_env

    # remove SSH_USE_STRONG_RNG from files
    ssh_remove_strong_rng $SSHDCONF
    ssh_remove_strong_rng $CCCONF

    restart_service sshd
}
