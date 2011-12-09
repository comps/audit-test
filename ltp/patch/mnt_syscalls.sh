#!/bin/sh
# 12/2011 created by T.N. Santhosh <santhosh.tn@hp.com>
# DESCRIPTION: This script uses block loopback devices to test mount feature.

LOOP_IMG="block.img"
LOOP_DEV="/dev/loop0"

create_loopback_system()
{
        echo "Creating a loopback filesystem"
        dd if=/dev/zero of=$LOOP_IMG count=2880
	if [ $? -ne 0 ]; then
		echo "Error creating $LOOP_IMG for device $LOOP_DEV"; exit 1
	fi

        losetup $LOOP_DEV $LOOP_IMG
	if [ $? -ne 0 ]; then
		echo "Error setting up $LOOP_DEV"; exit 1
	fi

        mke2fs -F $LOOP_DEV

	# detach the loop device
        losetup -d $LOOP_DEV
	if [ $? -ne 0 ]; then
		echo "Error detaching $LOOP_DEV"; exit 1
	fi
}

cleanup()
{
	# detach the loop device
	losetup -d $LOOP_DEV
	if [ $? -ne 0 ]; then
		echo "Error detaching $LOOP_DEV"; exit 1
	fi

	# remove the loop file
        rm -f $LOOP_IMG
}

create_loopback_system
losetup $LOOP_DEV $LOOP_IMG

cd `dirname $0`
cd ..
./runltp -d ${PWD}/tmp -f ${PWD}/runtest/mnt_syscalls -q "$@"

cleanup

