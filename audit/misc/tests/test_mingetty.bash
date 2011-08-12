#!/bin/bash
###############################################################################
#   Copyright 2011 atsec information security corp.
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
# To verify mingetty

source misc_functions.bash || exit 2

# turn off screen in /etc/profile
backup /etc/profile
sed -i -e 's/\[ -w .*\]/false/' /etc/profile

# if in LSPP mode, map the TEST_USER to staff_u
if [[ $PPROFILE == "lspp" ]]; then
	semanage login -d $TEST_USER
	semanage login -a -s staff_u $TEST_USER
fi

# calling login in this manner leaves an entry in /var/run/utmp
# use backup (and automatic restore) to work around this
backup /var/run/utmp

# $1: full tty name
# $2: user name
# $3: password
testlogin() {
	echo "Testing login on TTY $1: "
	err=0
	# username
	./do_tty $1 $2\\n
	sleep 1
	# password
	./do_tty $1 $3\\n
	sleep 1
	# newline (mls may ask a question at login)
	./do_tty $1 \\n
	sleep 2

	ttyname=$(basename $1)
	ret=$(ps -efa | grep $ttyname | grep $2 | grep -q bash)
	if [ "$?" -ne "0" ]; then
		echo
		echo "ERROR - bash shell not found after login attempt"
		ps -efa | grep $ttyname
		let err=$err+1
	else
		echo "Checking permissions of device to be $1 $2 tty 620/600"
		checkperm $1 $2 tty 6[20]0
		if [ "$?" -ne "0" ]; then
			let err=$err+1
		fi
	fi

	./do_tty $1 exit\\n
	sleep 2

	ret=$(ps -efa | grep $ttyname | grep root | grep -q mingetty)
	if [ "$?" -ne "0" ]; then
		echo
		echo "ERROR - mingetty not found after logout attempt"
		ps -efa | grep $ttyname | grep $2
		let err=$err+1
	fi

	if [ "$err" -ne "0" ]; then
		echo "FAIL"
		return $err
	else
		echo "PASS"
		return 0
	fi
}

# $1: full tty name
testgetty() {
	echo "Testing mingetty listening on TTY $1: "
	ret=$(lsof | grep $1 | grep getty | wc -l)
	if [ "$ret" -ne "3" ]; then
		echo "FAIL"
		return 1
	else
		echo "PASS"
		return 0
	fi
}

# $1: full tty name
# $2: owning user
# $3: owning group
# $4: permissions in octal (can be a regex)
checkperm() {
	err=0
	user=$(stat -c %U $1)
	if [ "$user" != "$2" ];then
		echo
		echo "ERROR - Owning user is: $user"
		let err=$err+1
	fi
	
	group=$(stat -c %G $1)
	if [ "$group" != "$3" ];then
		echo
		echo "ERROR - Owning group is: $group"
		let err=$err+1
	fi
	
	perm=$(stat -c %a $1)

	[[ "$perm" =~ $4 ]]
	if [ "$?" -ne "0" ]; then
		echo
		echo "ERROR - Permissions are: $perm"
		let err=$err+1
	fi

	if [ "$err" -ne "0" ]; then
		return 1
	else
		return 0
	fi
}


#  Test case: Verification of the correct operation of mingetty. The following
#             functionality is checked for each TTY out of /dev/tty1 through 6:
#             1. Permissions of TTY root:root 600
#             2. Log in with eal user
#             3. Check that bash is spawned for eal user
#             4. Permission of TTY eal:tty 620 (or more restrictive)
#             5. Logout
#             6. Permissions of TTY root:root 600
#  Execute (with root privileges): make
#  Cleanup: make clean

(
	for i in 1 2 3 4 5 6; do
		testgetty /dev/tty$i
		if [ "$?" -ne "0" ]; then
			echo "TTY /dev/tty$i is not owned by mingetty, skipping mingetty test"
			continue
		fi
		checkperm /dev/tty$i root root 600
		if [ "$?" -ne "0" ]; then
			echo "Checking permissions of TTY $1: user $2, group $3, permission $4: FAILED"		
			exit_fail
		fi
		
		testlogin /dev/tty$i $TEST_USER $TEST_USER_PASSWD
		if [ "$?" -ne "0" ]; then
			exit_fail
		fi
		checkperm /dev/tty$i root root 600
		if [ "$?" -ne "0" ]; then
			echo "Checking permissions of TTY $1: user $2, group $3, permission $4: FAILED"		
			exit_fail
		fi
	done

	exit_pass
)

