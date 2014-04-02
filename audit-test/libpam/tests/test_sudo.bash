#!/bin/bash

# Test case written by Stephan Mueller <smueller@atsec.com>
# Copyright (c) 2010 atsec information security
#
# Purpose:		Testing of sudo execution, authentication
#                       and sudoers enforcement
#
# Expected result:      See the test definitions below for a description of
#                       the expected test results.
#
# Test execution:       execute $0 as root
#
# The following matrices define the test units specified with this test.
# To read a matrix, use the following column definitions:
#	1st column: this lists the request
#	2nd column: this lists the configuration in sudoers
#		    between 1st and 2nd column, the equality between the
#		    value in the requested command and the value found in
#		    sudoers is specified
#	3rd column: marks the epxected result - either operation allowed or not
#	4th column: references the test case (you find it by searching the
#		    test definitions and looking for the suffix referenced
#		    by this column
#
# Note: any other component of sudoers or the request which are not
# tested are set such that the request is allowed
#
# Testing User_Alias
#User		User		Operation	Test
#requesting	configured	allowed?	case
#operation	in sudoers
#----------------------------------------------------
#User	eq	User		y		u1
#User 	!eq	User		n		u2
#User 	in	Group		y		u3
#User 	!in	Group		n		u4

# Testing Runas_Alias
#Target user	Target user	Operation	Test
#requested	in sudoers	allowed?	case
#-----------------------------------------------------
#User	eq	User		y		u1
#User	!eq	User		n		t2
#User	in	Group		y		t3
#User	!in	Group		n		t4

# Testing Cmd_Alias
#CMD		CMD		Operation	Test
#requested	in sudoers	allowed?	case
#---------------------------------------------------
#cmd	eq	cmd		y		u1
#cmd	!eq	cmd		n		c2
#cmd	in	dir		y		c3
#cmd	!in	dir		n		c4
#
# Testing password enforcement
#Password	Password	Operation	Test
#in request	setting in	allowed?	case
#		sudoers
#----------------------------------------------------
#right pass	default		y		u1
#wrong pass	default		n		p2
#right pass	NOPASSWD:	y		p3
#wrong pass	NOPASSWD:	y		p4
#
TESTS="u1 u2 u3 u4 t2 t3 t4 c2 c3 c4 p2 p3 p4"

# DO NOT CHANGE
USERG="sudouser1"
USERG_ID=12345
USERO="sudouser2"
USERO_ID=12346
USERT="sudotarget"
USERT_ID=12347
GROUP="sudogroup"
PASS="Tad6osBijy"
PASSENC='$6$Rpvtlluu$K63QZN9do4I03/uaKYVFxe3d7CZHOCUsAQNs7F5CQ.b.HJgcGaLOx6qRepDNko4xFxO0VFk4OEQzXHGBAtfHe0'
# DO NOT CHANGE

# User definitions:
#	USERG: member of group GROUP (should be used as requesting user)
#	USERT: member of group GROUP (should be used as target user)
#	USERO: not a member of GROUP (may be used as requesting and target user)
USER_SUDO_u1=$USERG
USER_EXEC_u1=$USERG
RUN_SUDO_u1=$USERT
RUN_EXEC_u1=$USERT
CMD_SUDO_u1="/usr/bin/id"
CMD_EXEC_u1="/usr/bin/id -u"
CMD_RES_u1=$USERT_ID
CMD_RET_u1=0

USER_SUDO_u2=$USERG
USER_EXEC_u2=$USERO
RUN_SUDO_u2=$USERT
RUN_EXEC_u2=$USERT
CMD_SUDO_u2="/usr/bin/id"
CMD_EXEC_u2="/usr/bin/id -u"
CMD_RES_u2=""
CMD_RET_u2=1

USER_SUDO_u3="%$GROUP"
USER_EXEC_u3=$USERG
RUN_SUDO_u3=$USERT
RUN_EXEC_u3=$USERT
CMD_SUDO_u3="/usr/bin/id"
CMD_EXEC_u3="/usr/bin/id -u"
CMD_RES_u3=$USERT_ID
CMD_RET_u3=0

USER_SUDO_u4="%$GROUP"
USER_EXEC_u4=$USERO
RUN_SUDO_u4=$USERT
RUN_EXEC_u4=$USERT
CMD_SUDO_u4="/usr/bin/id"
CMD_EXEC_u4="/usr/bin/id -u"
CMD_RES_u4=""
CMD_RET_u4=1

USER_SUDO_t2=$USERO
USER_EXEC_t2=$USERO
RUN_SUDO_t2=$USERT
RUN_EXEC_t2=$USERO
CMD_SUDO_t2="/usr/bin/id"
CMD_EXEC_t2="/usr/bin/id -u"
CMD_RES_t2=""
CMD_RET_t2=1

USER_SUDO_t3=$USERO
USER_EXEC_t3=$USERO
RUN_SUDO_t3="%$GROUP"
RUN_EXEC_t3=$USERT
CMD_SUDO_t3="/usr/bin/id"
CMD_EXEC_t3="/usr/bin/id -u"
CMD_RES_t3=$USERT_ID
CMD_RET_t3=0

USER_SUDO_t4=$USERO
USER_EXEC_t4=$USERO
RUN_SUDO_t4="%$GROUP"
RUN_EXEC_t4=$USERO
CMD_SUDO_t4="/usr/bin/id"
CMD_EXEC_t4="/usr/bin/id -u"
CMD_RES_t4=""
CMD_RET_t4=1

USER_SUDO_c2=$USERG
USER_EXEC_c2=$USERG
RUN_SUDO_c2=$USERT
RUN_EXEC_c2=$USERT
CMD_SUDO_c2="/usr/bin/id"
CMD_EXEC_c2="/bin/ls"
CMD_RES_c2=""
CMD_RET_c2=1

USER_SUDO_c3=$USERG
USER_EXEC_c3=$USERG
RUN_SUDO_c3=$USERT
RUN_EXEC_c3=$USERT
CMD_SUDO_c3="/usr/bin/"
CMD_EXEC_c3="/usr/bin/id -u"
CMD_RES_c3=$USERT_ID
CMD_RET_c3=0

USER_SUDO_c4=$USERG
USER_EXEC_c4=$USERG
RUN_SUDO_c4=$USERT
RUN_EXEC_c4=$USERT
CMD_SUDO_c4="/bin/"
CMD_EXEC_c4="/usr/bin/id -u"
CMD_RES_c4=""
CMD_RET_c4=1

USER_SUDO_p2=$USERG
USER_PASS_p2="wrongpass"
USER_EXEC_p2=$USERG
RUN_SUDO_p2=$USERT
RUN_EXEC_p2=$USERT
CMD_SUDO_p2="/usr/bin/id"
CMD_EXEC_p2="/usr/bin/id -u"
CMD_RES_p2=""
CMD_RET_p2=1

USER_SUDO_p3=$USERG
USER_EXEC_p3=$USERG
RUN_SUDO_p3=$USERT
RUN_EXEC_p3=$USERT
CMD_SUDO_p3="NOPASSWD: /usr/bin/id"
CMD_EXEC_p3="/usr/bin/id -u"
CMD_RES_p3=$USERT_ID
CMD_RET_p3=0

USER_SUDO_p4=$USERG
USER_PASS_p4="wrongpass"
USER_EXEC_p4=$USERG
RUN_SUDO_p4=$USERT
RUN_EXEC_p4=$USERT
CMD_SUDO_p4="NOPASSWD: /usr/bin/id"
CMD_EXEC_p4="/usr/bin/id -u"
CMD_RES_p4=$USERT_ID
CMD_RET_p4=0

########### no further test specification beyond this line ############

source pam_functions.bash || exit 2

setup_cleanup() {
	prepend_cleanup "rm -f /etc/sudoers.new"
	prepend_cleanup "groupdel $GROUP"
	prepend_cleanup "killall -9 -u $USERT; userdel -rf $USERT"
	prepend_cleanup "killall -9 -u $USERO; userdel -rf $USERO"
	prepend_cleanup "killall -9 -u $USERG; userdel -rf $USERG"
}

gen_user() {
	killall -9 -u $USERG
	killall -9 -u $USERO
	killall -9 -u $USERT
	userdel -rf $USERG 2> /dev/null
	userdel -rf $USERO 2> /dev/null
	userdel -rf $USERT 2> /dev/null
	groupdel $GROUP 2> /dev/null
	groupadd $GROUP
	useradd -u $USERG_ID -g $GROUP -p $PASSENC $USERG
	useradd -u $USERO_ID -p $PASSENC $USERO
	useradd -u $USERT_ID -g $GROUP -p $PASSENC $USERT
}

setup_sudoers() {
	local User_Alias=$1
	local Runas_Alias=$2
	shift; shift
	local Cmd_Alias=$@

	local perm=
	perm=$(stat -c %a /etc/sudoers)

	perl -ne 'print unless /#SUDO_TESTING_START/../#SUDO_TESTING_END/' \
		< /etc/sudoers | sed -e 's/^Defaults    requiretty/# Defaults    requiretty/' > /etc/sudoers.new
	# Only modify sudoers file when we are given some variables
	# if not, we basically clean up sudoers
	[ -n "$User_Alias" ] && {
		echo "#SUDO_TESTING_START" >> /etc/sudoers.new
		echo "User_Alias USER = $User_Alias" >> /etc/sudoers.new
		echo "Runas_Alias RUNAS = $Runas_Alias" >> /etc/sudoers.new
		echo "Defaults:USER timestamp_timeout=0" >> /etc/sudoers.new
		echo "USER  ALL = (RUNAS) $Cmd_Alias" >> /etc/sudoers.new
		echo "#SUDO_TESTING_END" >> /etc/sudoers.new
	}
	mv -f /etc/sudoers.new /etc/sudoers
	chmod $perm /etc/sudoers
}

testloop() {

	local res=""
	local ret=""
	local testfail=0
	local testpass=0
	local testno=0

	for i in $TESTS; do
		local USER_SUDO=""
		eval USER_SUDO=\$USER_SUDO_$i
		local USER_EXEC=""
		eval USER_EXEC=\$USER_EXEC_$i
		local RUN_SUDO=""
		eval RUN_SUDO=\$RUN_SUDO_$i
		local RUN_EXEC=""
		eval RUN_EXEC=\$RUN_EXEC_$i
		local CMD_SUDO=""
		eval CMD_SUDO=\$CMD_SUDO_$i
		local CMD_EXEC=""
		eval CMD_EXEC=\$CMD_EXEC_$i
		local CMD_RES=""
		eval CMD_RES=\$CMD_RES_$i
		local CMD_RET=""
		eval CMD_RET=\$CMD_RET_$i
		local USER_PASS=""
		eval USER_PASS=\$USER_PASS_$i

		[ -z "$USER_PASS" ] && USER_PASS=$PASS

		setup_sudoers $USER_SUDO $RUN_SUDO $CMD_SUDO
		res=$(su -c "echo $USER_PASS | sudo -S -u $RUN_EXEC $CMD_EXEC 2>/dev/null" $USER_EXEC)
		ret=$?
		let testno=$testno+1

		if [ "$res" = "$CMD_RES" -a "$ret" -eq "$CMD_RET" ]; then
			echo "Test $i PASSED"
			let testpass=$testpass+1
		else
			echo "Test $i: actual result output $res - expected $CMD_RES"
			echo "Test $i: actual return value $ret - expected $CMD_RET"
			echo "Test $i FAILED"
			let testfail=$testfail+1
		fi
	done

	echo "Number of tests executed: $testno"
	echo "Number of tests failed:   $testfail"
	echo "Number of tests passed:   $testpass"

	return $testfail

}

main() {
	setup_cleanup

	gen_user
	backup /etc/sudoers

	testloop
	if [ $? -gt 0 ]; then
		exit_fail
	else
		exit_pass
	fi
}

main

