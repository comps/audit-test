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

# globals
[ -z $LOOPDEV ] && LOOPDEV=$(losetup -f)

# Create new loop device. The paramters are optional. If you intend to pass
# only second paramater you need to supply the first too!
# $1 = loop device (default $LOOPDEV)
# $2 = size in MB (default 100MB)
function create_loop_device {
	# defaults
	LOOPSIZE="100"

	# if parameters given
	[ "x$1" != "x" ] && LOOPDEV=$1
	[ "x$2" != "x" ] && LOOPSIZE=$2


	# create the test file
	LOOPFILE=$(mktemp)
	dd if=/dev/zero of=$LOOPFILE bs=1M count=$LOOPSIZE || exit_error \
		"Error creating $LOOPFILE for device $LOOPDEV"

	# create loop device
	losetup $LOOPDEV $LOOPFILE || exit_error \
		"Error setting up $LOOPDEV"
}

# Umount, detach the loop device and remove the loop file.
# The paramter is optional.
# $1 = loop device (default $LOOPDEV)
function remove_loop_device {
	# if parameter given
	[ "x$1" != "x" ] && LOOPDEV=$1

	# umount the loop device
	umount $LOOPDEV

	# extract the file associated with the device
	LOOPFILE=$(losetup $LOOPDEV | sed 's|.*(\(/.*\))|\1|g')

	# detach the loop device
	losetup -d $LOOPDEV || exit_error "Error detaching $LOOPDEV"

	# remove the loopfile
	rm -f $LOOPFILE

}
