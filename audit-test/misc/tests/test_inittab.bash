#!/bin/bash
###############################################################################
#   Copyright (C) 2011 Silicon Graphics International Corp. (SGI)
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
###############################################################################
# 
# PURPOSE:
# To verify inittab

source misc_functions.bash || exit 2

# if in LSPP mode, map the TEST_USER to staff_u
if [[ $PPROFILE == "lspp" ]]; then
	semanage login -d $TEST_USER
	semanage login -a -s staff_u $TEST_USER
	append_cleanup user_cleanup
fi

#  Test case: Verification of the correct operation of inittab. This 
#             tests the functionality of init to spawn jobs as define by 
#             the /etc/init/*.conf configuration files.

(
	# Copy the file inittab-test.conf to the /etc/init/ directory.
	cp inittab-test.conf /etc/init/
	if [ "$?" -ne "0" ]; then
		echo "Copying inittab-test.conf to /etc/init/ FAILED"
		exit_fail
	fi

	# Start inittab-test job.
	initctl start inittab-test
	if [ "$?" -ne "0" ]; then
		echo "initctl start inittab-test FAILED"
		exit_fail
	fi

	# Check for inittab-test process ('/bin/sleep 300').
	ps -efa | grep "[/]bin/sleep 300"
	if [ "$?" -ne "0" ]; then
		echo "inittab-test process not found: FAILED"		
		exit_fail
	fi

	# Stop inittab-test job.
	initctl stop inittab-test
	if [ "$?" -ne "0" ]; then
		echo "initctl stop inittab-test FAILED"
		exit_fail
	fi

	# Check for inittab-test process ('/bin/sleep 300').
	ps -efa | grep "[/]bin/sleep 300"
	if [ "$?" -eq "0" ]; then
		echo "inittab-test process was not stopped: FAILED"		
		exit_fail
	fi
	
	# Remove the /etc/init/inittab-test.conf file.
	rm /etc/init/inittab-test.conf
	if [ "$?" -ne "0" ]; then
		echo "Remove /etc/init/inittab-test.conf FAILED"
		exit_fail
	fi

	exit_pass
)

