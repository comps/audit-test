###############################################################################
#   Copyright (c) 2011 Red Hat, Inc. All rights reserved.
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
# Description:
# Loop device helper functions
#

# default loop device to use
[ -z $LOOPDEV ] && LOOPDEV=$(losetup -f)

# set default expect timeout to 120s
TIMEOUT=120

# Create LUKS on $LOOPDEV with $LUKSPASS password
# $1 - password to use
function create_luks {
	expect -c "
		set timeout $TIMEOUT
		spawn cryptsetup luksFormat $LOOPDEV
		expect {Are you sure} {send \"YES\r\"}
		expect {Enter LUKS} {send \"$1\r\"}
		expect {Verify} {send \"$1\r\"}
		expect eof
	"
}

# Close LUKS on $LOOPDEV
# $1 - luks device
function close_luks {
	cryptsetup luksClose $LUKSDEV
}

# Check if LUKS on $LOOPDEV created with correct parameters and uses
# given count of key slots
# $1 - number of key slots to check
function check_luks {
	# check if mandatory paramter given
	[ "x$1" = "x" ] && exit_error "Error: no parameter give for check_luks"

	# dump the LUKS device
	TMP=$(mktemp)
	cryptsetup luksDump $LOOPDEV &> $TMP

	# Check for correct parameters
	egrep "Cipher name.*aes" $TMP || exit_fail "Failed check on cipher name"
	egrep "Cipher mode.*cbc-essiv:sha256" $TMP || \
		exit_fail "Failed check on cipher mode"
	egrep "Hash spec.*sha1" $TMP || exit_fail "Failed check on hash spec"

	# Check for correct count of used key slots
	CNT=$(egrep "Key Slot.*ENABLED" $TMP | wc -l)
	[ $CNT -ne $1 ] && \
		exit_fail "Incorrect count of key slots: $CNT (expected $1)"

}

# Add new key passphrase give as paramter to the $LUKSDEV device
# $1 - any current passphrase
# $2 - passphrase for the new key
function addkey_luks {
	# check if mandatory parameter given
	[ "x$1" = "x" ] && \
		exit_error "Error: no current passphrase given to addkey_luks"
	[ "x$2" = "x" ] && \
		exit_error "Error: no new passphrase given to addkey_luks"

	# add new key slot
	expect -c "
		set timeout $TIMEOUT
		spawn cryptsetup luksAddKey $LOOPDEV
		expect {Enter any} {send \"$1\r\"}
		expect {Enter new} {send \"$2\r\"}
		expect {Verify} {send \"$2\r\"}
		expect eof
	"
}

# Create dm-crypt mapping for give LUKS device
# $1 - device name
# $2 - any passphrase for opening LUKS
# return - 0 if open successful, else 1
function open_luks {
	# check if mandatory parameter given
	[ "x$1" = "x" ] && \
		exit_error "Error: no device name given to open_luks"
	[ "x$2" = "x" ] && \
		exit_error "Error: no passphrase given to open_luks"

	# try to open the device
	expect -c "
		set timeout $TIMEOUT
		spawn cryptsetup luksOpen $LOOPDEV $1
		expect {Enter passphrase} {send \"$2\r\"}
		expect eof { exit 0 }
		exit 1
	"

	# open failed
	[ $? -ne 0 ] && return 1

	# open successful
	return 0
}

# Remove dm-crypt mapping for given device
# $1 - device to remove
function close_luks {
	# check if mandatory parameter given
	[ "x$1" = "x" ] && \
		exit_error "Error: no device name given to close_luks"

	# close the device
	cryptsetup luksClose $1

	# check if close successful
	[ $? -ne 0 ] && exit_fail "Failed to close LUKS device"

}
