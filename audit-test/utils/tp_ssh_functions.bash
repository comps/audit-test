###############################################################################
#   Copyright (c) 2011, 2014 Red Hat, Inc. All rights reserved.
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
# Various SSH helper functions mainly for crypto bucket.
#

source functions.bash || exit 2

#### global configuration
# default backup folder, needs to be writable by the user!
BFOLDER="/tmp"
RND=$RANDOM
# default expect timeout 10 minutes - in case of not enough entropy
TIMEOUT=600
# ssh log location from rsyslog.conf
SSHDLOG=$(grep ^authpriv /etc/rsyslog.conf | awk '{print $2}')

# Get ssh log mark used to limit grep in sshd
function get_sshdlog_mark() {
    echo "$(stat -c %s $SSHDLOG)"
}

# Remove SSH_USE_STRONG_RNG from environment
function ssh_remove_strong_rng_env {
    export -n SSH_USE_STRONG_RNG
    prepend_cleanup "export SSH_USE_STRONG_RNG=12; readonly SSH_USE_STRONG_RNG"
}

# Remove SSH_USE_STRONG_RNG exporting from give file
# Does nothing if given file does not exist
# $1 - file
function ssh_remove_strong_rng {
    [ -z "$1" ] && exit_error "No file given for $FUNCNAME"
    [ -f $1 ] || return

    sed -i "s/.*SSH_USE_STRONG_RNG.*//g" $1
}

# Remove sleep calls from given file
# Does nothing if given file does not exist
# $1 - file
function ssh_remove_screen {
    [ -z "$1" ] && exit_error "No file given for $FUNCNAME"
    [ -f $1 ] || return

    sed -i "s/.*sleep [0-9]\+.*//g; s/.*exec.*SCREENEXEC.*//g" $1
}

# Backup the .ssh directory of the specified user. The backup
# is created in $BFOLDER folder under 'user_$RND_sshbackup.tgz'.
# $1 - user
function ssh_backup_home {
    # check if required user specified
    [ -z "$1" ] && exit_error "Error: no user given for $FUNCNAME"

    # if nothing to backup bail out
    /bin/su - -c "test -d .ssh" $1 || return

    # creating the backup is ran under the user because getting home folder
    # in MLS can be tricky
    BFILE="$BFOLDER/$1_${RND}_sshbackup.tgz"
    /bin/su - -c "tar --selinux -czpv -f $BFILE .ssh" $1 || \
        exit_error "Error creating .ssh backup $BFILE"
}

# Cleanup keys or the whole .ssh directory of the specified user.
# If -c passed as first parameter user is currentuser use the current user.
# $1 - user or -c
# $2 - password
# $3 - optional key to cleanup
function ssh_cleanup_home {
    if [ "$1" = "-r" ]; then
        # check required paramaters
        [[ -n "$2" ]] && [[ ! "$2" =~ (ecdsa|dsa|rsa) ]] && \
            error "Error: uknown key type $2 for $FUNCNAME"

        # backup .ssh or specified public/private keys
        if [ -n "$2" ]; then
            backup ~/.ssh/id_$2{,.pub}
            rm -f ~/.ssh/id_$2{,.pub}
        else
            backup ~/.ssh
            rm -rf ~/.ssh
        fi
    else
        # check required parameters
        [ -z "$1" ] && exit_error "Error: no user given for $FUNCNAME"
        [ -z "$2" ] && exit_error "Error: no password given for $FUNCNAME"
        [[ -n "$3" ]] && [[ ! "$3" =~ (ecdsa|dsa|rsa) ]] && \
            error "Error: uknown key type $3 for $FUNCNAME"

        # backup .ssh or specified public/private keys
        if [ -n "$3" ]; then
            ssh_cmd $1 $2 "rm -f .ssh/id_$3{,.pub}"
        else
            ssh_cmd $1 $2 "rm -rf .ssh"
        fi
    fi
}

# Restore the .ssh directory of the specified user.
# If .ssh folder exitsts wipe it before the restore.
# $1 - user
function ssh_restore_home {
    # check if required user specified
    [ -z "$1" ] && exit_error "Error: no user given for $FUNCNAME"

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

# Create auth key for ssh with given or none passphrase in default location
# $1 - user
# $2 - user password
# $3 - type (rsa/dsa)
# $4 - number of bits in the key
# $5 - passphrase for key (optional)
function ssh_create_key {
    if [ "$1" = "-r" ]; then
        [ -z "$2" ] && exit_error "Error: no key type given for $FUNCNAME"
        [ -z "$3" ] && exit_error "Error: no key size given for $FUNCNAME"

        # generate the requested keys for running user
        ssh-keygen -m PEM -t $2 -b $3 -N "$4" -f $HOME/.ssh/id_$2
    else
        # check if required user specified
        [ -z "$1" ] && exit_error "Error: no user given for $FUNCNAME"
        [ -z "$2" ] && exit_error "Error: no password given for $FUNCNAME"
        [ -z "$3" ] && exit_error "Error: no key type given for $FUNCNAME"
        [ -z "$4" ] && exit_error "Error: no key size given for $FUNCNAME"

        # generate the keys
        expect -c "set timeout $TIMEOUT
            spawn ssh $1@localhost \"ssh-keygen -m PEM -t $3 -b $4 -N '$5' -f \\\$(pwd)/.ssh/id_$3\"
            expect {
                {yes/no} { after 100; send -- \"yes\r\"; exp_continue }
                {assword:} { after 100; send -- \"$2\r\"; exp_continue }
                {randomart} {
                    expect eof { exit 0 }
                    default { exit 1 }
                }
                default { exit 1 }
            }"
    fi
    return $?
}

# Check ssh key for correct cipher and key length using openssl
# $1 - user
# $2 - user password
# $4 - number of bits in the key
function ssh_check_key {
    # check if required user specified
    [ -z "$1" ] && exit_error "Error: no user given for $FUNCNAME"
    [ -z "$2" ] && exit_error "Error: no password given for $FUNCNAME"
    [ -z "$3" ] && exit_error "Error: no key type given for $FUNCNAME"
    [ -z "$4" ] && exit_error "Error: no key size given for $FUNCNAME"

    # check key
    [ "$3" = "ecdsa" ] && cryptalg="ec" || cryptalg=$3
    ssh_cmd $1 $2 "openssl $cryptalg -in /home/$1/.ssh/id_$3 -text" | grep "$4 bit" || \
        exit_fail "Key /home/$1/.ssh/id_$3 has incorrect size"
}


# Run command via ssh.
# $1 - user
# $2 - pas
# $3 - cmd
# return value - return value of cmd
function ssh_cmd {
    # check if required params specified
    [ -z "$1" ] && exit_error "Error: no user for $FUNCNAME"
    [ -z "$2" ] && exit_error "Error: no password for $FUNCNAME"
    [ -z "$3" ] && exit_error "Error: no comamnd for $FUNCNAME"

    expect -c "set timeout $TIMEOUT
        spawn ssh $1@localhost \"$3\"
        expect {
            {yes/no} { after 100; send -- yes\r; exp_continue }
            {assword:} { after 100; send -- $2\r }
            eof { catch wait result; exit [lindex \$result 3] }
        }
        expect {
            eof { catch wait result; exit [lindex \$result 3] }
            {assword:} { exit 2 }
        }
        exit 1"

    return $?
}

# Run command via ssh as root in mls mode.
# $1 - cmd
function ssh_mls_cmd {

    # check if required params specified
    [ -z "$1" ] && exit_error "Error: no comamnd for $FUNCNAME"

    local M="${RANDOM}${RANDOM}${RANDOM}"
    local rc=0

    expect -c "set timeout $TIMEOUT
      spawn -nottycopy -nottyinit ssh eal@localhost
      expect {
        {*yes/no} { 
          after 1000 { send -- yes\r }
          exp_continue 
        }
        {*assword} { after 1000 { send -- $PASSWD\r } }
        default { exit 2 }
      }
      expect {
        {*get default type} { exit 3 }
        {*eal} { 
          after 1000 { send -- \"newrole -r lspp_test_r\n\" }
          exp_continue 
        }
        {*assword} { after 1000 { send -- \"$PASSWD\r\" } }
        default { exit 4 }
      }
      expect {
        {*eal} {
          after 1000 { send -- \"/bin/su -\r\" }
          exp_continue 
        }
        {*assword} { after 1000 { send -- \"$PASSWD\r\" } }
        default { exit 5 }
      }
      expect {
        {*root} { 
          after 1000 {
            send -- \"$1 && (echo -n $M && echo PASS) || (echo -n $M && echo FAIL)\r\"
          }
        }
        default { exit 6 }
      }
      expect {
        {${M}PASS} { exit 0 }
        {${M}FAIL} { exit 1 }
      }
      exit 7"

    rc=$?
    [ $rc -ne 0 ] && exit_fail "Failed to run $1 as root in mls mode (rc=$rc)" && return 1

    return 0
}

# Run command via ssh in mls mode.
# $1 - cmd
# $2 - user (optional)
# $3 - opts (optional)
function ssh_mls_cmd_user {
    local CUSR= 
    local COPTS="$3"

    # check if required params specified
    [ -z "$1" ] && exit_error "Error: no comamnd for $FUNCNAME"
    [ -z "$2" ] && CUSR="eal" || CUSR="$2"

    local M="${RANDOM}${RANDOM}${RANDOM}"

    # when expecting user disregard MLS user specification
    # i.e. eal/staff_r/s3 .. expect only eal
    expect -c "set timeout $TIMEOUT
      set ret 5     
      spawn ssh $COPTS $CUSR@127.0.0.1
      expect {
        {*yes/no} { after 100; send -- yes\r; exp_continue }
        {*assword} { after 100; send -- $PASSWD\r }
        default { exit 1 }
      }
      expect {
        {*${CUSR%%/*}} { after 100; send -- \"$1 && (echo -n $M && echo PASS) || (echo -n $M && echo FAIL); exit\r\" }
        default { exit 2 }
      }
      expect {
        {${M}PASS} { set ret 0 }
        {${M}FAIL} { set ret 1 }
        default { exit 3 }
      }
      expect {
        eof { exit \$ret }
        default { exit 4 }
      }
      "

    return $?
}

# Check permissions and selinux context of the .ssh directory and the
# underlying public and private key
# $1 - user
# $2 - password
# $3 - type (rsa/dsa)
# $4 - optional - if any string specified check also authorized keys
function ssh_check_home {
    # check if required params specified
    [ -z "$1" ] && exit_error "Error: no user for $FUNCNAME"
    [ -z "$2" ] && exit_error "Error: no password for $FUNCNAME"
    [ -z "$3" ] && exit_error "Error: no key type for $FUNCNAME"

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

    if [ -n "$4" ]; then
        ssh_cmd $1 $2 "stat -c '%a' /home/$1/.ssh/authorized_keys" | grep 600 || \
            exit_fail "File authorized_keys has bad permissions"
        ssh_cmd $1 $2 "stat -c '%C' /home/$1/.ssh/authorized_keys" | grep ssh_home_t || \
            exit_fail "File authorized_keys has bad SELinux context"
    fi
}

# Copy key from user1 to user2 using ssh-copy-id and test if successful
# $1 - source user
# $2 - source user password
# $3 - destination user
# $4 - destination user password
# $5 - rsa/dsa - specifies key to copy from .ssh folder
function ssh_copy_key {
    if [ "$1" = "-r" ]; then
        # check if required params specified
        [ -z "$2" ] && exit_error "Error: no dst user for $FUNCNAME" || \
            local DUSR=$2
        [ -z "$3" ] && exit_error "Error: no dst user password for $FUNCNAME" || \
            local DPASS=$3
        [ -z "$4" ] && exit_error "Error: no key for $FUNCNAME" || \
            local DKEY=$4
    else
        # check if required params specified
        [ -z "$1" ] && exit_error "Error: no src user for $FUNCNAME" || \
            local SUSR=$1
        [ -z "$2" ] && exit_error "Error: no src user password for $FUNCNAME" || \
            local SPASS=$2
        [ -z "$3" ] && exit_error "Error: no dst user for $FUNCNAME" || \
            local DUSR=$3
        [ -z "$4" ] && exit_error "Error: no dst user password for $FUNCNAME" || \
            local DPASS=$4
        [ -z "$5" ] && exit_error "Error: no key for $FUNCNAME" || \
            local DKEY=$5
    fi

    # copy key from user1 to user2
    [ -n "$SUSR" ] && PRIVKEY="/home/$SUSR/.ssh/id_$DKEY" || \
        PRIVKEY="$HOME/.ssh/id_$DKEY"
    expect -c "set timeout $TIMEOUT
        if {\"$SUSR\" ne \"\"} {
            spawn ssh $1@localhost
            expect {
                {yes/no} { after 100; send -- yes\r; exp_continue }
                {assword:} { after 100; send -- $2\r; exp_continue }
                {$1}
            }
            after 100; send -- \"ssh-copy-id -i $PRIVKEY $DUSR@localhost\r\"
        } else {
            spawn ssh-copy-id -i $PRIVKEY $DUSR@localhost
        }
        expect {
            {yes/no} { after 100; send -- yes\r; exp_continue }
            {assword:} { after 100; send -- $DPASS\r }
            default { exit 1 }
        }
        expect {try logging}
        after 100; send -- \"exit\r\"
        expect eof { exit 0 }
        exit 2"

    [ $? -ne 0 ] && exit_fail "Failed to copy $PRIVKEY from ${SUSR:-$USER} to $DUSR"
}

# Connect from user1 to user2 with pubkey authentication and test if successful
# $1 - source user
# $2 - source user password
# $3 - destination user
# $4 - key passphrase (optional)
# return value - 0 on success
#                1 if connecting to unknown host
#                2 if for password asked
#                3 if timeout
#                4 should never happen
#                5 if permission denied on pubkey only auth
#                6 if account expired
#                7 if password change needed
function ssh_connect_key {
    local RET=

    # check if required params specified
    [ -z "$1" ] && exit_error "Error: no src user for $FUNCNAME"
    [ -z "$2" ] && exit_error "Error: no src user password for $FUNCNAME"
    [ -z "$3" ] && exit_error "Error: no dst user for $FUNCNAME"

    # connect from user $1 to user $3
    expect -c "set timeout $TIMEOUT
        spawn ssh $1@localhost
        expect {
            {yes/no} { after 100; send -- yes\r; exp_continue }
            {assword:} { after 100; send -- $2\r; exp_continue }
            {$1}
        }
        after 100; send -- \"ssh -o PasswordAuthentication=no $3@localhost whoami\r\"
        expect {
            {yes/no} { set ret 1 }
            -re \"$3\\\r\" { set ret 0 }
            {*assword:} { set ret 2; send BADPASS\r }
            {passphrase} { after 100; send -- $4\r; exp_continue }
            {Permission denied (publickey)} { set ret 5 }
            {Your account has expired} { set ret 6 }
            {Your password has expired} { set ret 7 }
            default { set ret 3 }
        }
	expect {
		{*assword:} { send BADPASS\r; exp_continue }
		{*$1} { send exit\r; exp_continue }
		eof { exit \$ret }
	}
        exit 4"; RET=$?

    # make sure faillock is the way
    faillock --reset --user $1
    faillock --reset --user $3

    # return set ret code of expect
    return $RET
}

# Connect from user1 to user2 with password and test if successful
# $1 - source user
# $2 - source user password
# $3 - destination user
# $4 - destination user password
# $5 - optional string passed to ssh
# return value - 0 success
#                1 other timeout
#                2 permission denied
#                3 unable to negotiate key exchange method
#                4 account expired
#                5 no matching mac found
#                6 no matching cipher found
#                7 no host key known
#                8 uknown cipher type
#                9 read from socket failed
#               10 unexpected eof
#               11 required to change password
function ssh_connect_pass {
    # check if required params specified
    [ -z "$1" ] && exit_error "Error: no src user for $FUNCNAME"
    [ -z "$2" ] && exit_error "Error: no src user password for $FUNCNAME"
    [ -z "$3" ] && exit_error "Error: no dst user for $FUNCNAME"
    [ -z "$4" ] && exit_error "Error: no dst user password for $FUNCNAME"

    # connect from user $1 to user $3
    expect -c "set timeout $TIMEOUT
        spawn ssh $1@localhost
        expect {
            {Read from socket failed} { exit 9 }
            {yes/no} { after 100; send -- yes\r; exp_continue }
            {assword:} { after 100; send -- $2\r; exp_continue }
            -nocase {no matching mac found} { exit 5 }
            {no matching key exchange method found} { exit 3 }
            eof { exit 10 }
            {$1}
        }
        after 100; send -- \"ssh $5 $3@localhost whoami\r\"
        expect {
            {yes/no} { after 100; send -- yes\r; exp_continue }
            {assword:} { after 100; send -- $4\r }
            {passphrase} { after 100; send -- INCORRECTPASS\r; exp_continue }
            {Unable to negotiate a key exchange method} { exit 3 }
            {no matching key exchange method found} { exit 3 }
            {Your password has expired} { exit 4 }
            {You are required to change} { exit 11 }
            {You are required to change your password} { exit 11 }
            -nocase {no matching mac found} { exit 5 }
            {Unknown cipher type} { exit 8 }
            {no matching cipher found} { exit 6 }
            -re \"No.*host key is known\" { exit 7 }
            eof { exit 10 }
            default { exit 1 }
        }
        expect {
            {Your account has expired} { exit 4 }
            {Password expired} { exit 4 }
            {Your password has expired} { exit 11 }
            {You are required to change your password} { exit 11 }
            {assword:} { after 100; send -- $4\r; exp_continue }
            {Permission denied} { exit 2 }
            -re \"$3\\\r\" { exit 0 }
        }"

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
    [ -z "$1" ] && exit_error "Error: no log mark given for $FUNCNAME"
    [ "$2" = "fail" ] && FAIL=yes
    [ -n "$3" ] && local GROKCIPHER="msg_1=~$3"

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
    local MPROFILE="/etc/profile"
    local SSHDCONF="/etc/sysconfig/sshd"
    local CCCONF="/etc/profile.d/cc-configuration.sh"

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

# This function sets the given option to the given value
# in ssh config file. If the option already exists it changes
# the option value.
# $1 - option to change/add
# $2 - value of the option
function sshd_config_set {
    [ -z "$1" ] && exit_error "Error: no option to change/add er for $FUNCNAME"
    [ -z "$2" ] && exit_error "Error: no option value for $FUNCNAME"

    local SSHDCONF="/etc/ssh/sshd_config"
    prepend_cleanup "restart_service sshd"
    backup $SSHDCONF

    if egrep -q "^$1" $SSHDCONF; then
        sed -i "s/^$1.*$/$1 $2/g" $SSHDCONF
    else
        printf "\n%s %s" "$1" "$2" >> $SSHDCONF
    fi

    # test if option set correctly
    egrep -q "^${1}.*$2" $SSHDCONF || \
        exit_error "Failed to set option in $FUNCNAME"

    restart_service sshd
}

# Expire password of local user
function expire_account {
    [ -z "$1" ] && exit_error "Error: no user given for $FUNCNAME"

    chage -E 0 $1 || exit_error "Failed to expire account $1"
    prepend_cleanup "chage -E -1 $1"
}

# Expire password of local user
function expire_password {
    [ -z "$1" ] && exit_error "Error: no user given for $FUNCNAME"

    chage -d -1 $1 || exit_error "Failed to set last password change"
    chage -M 0 $1 || exit_error "Failed to set password maximum validity"
    prepend_cleanup "chage -M -1 $1"
}
