#!/bin/bash
#
# =============================================================================
# (c) Copyright Hewlett-Packard Development Company, L.P., 2007
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of version 2 the GNU General Public License as
#   published by the Free Software Foundation.
#   
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#   
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
# =============================================================================
#

source testcase.bash || exit 2

######################################################################
# global variables
######################################################################

unset log_mark
unset auid subj gensubj obj_nolevel filelist dirlist items
auid=$(</proc/self/loginuid)
#
# if any of the below change, the test_context.fc file much change
#
testbase=/tmp/eal4_testing
filelist="s0_testfile s2_testfile s15_testfile"
dirlist="s0_testdir s2_testdir s15_testdir"
items=6
subj=staff_u:lspp_test_r:lspp_harness_t:s0-s15:c0.c1023
gensubj=staff_u:lspp_test_r:lspp_test_generic_t:s0-s15:c0.c1023
obj_nolevel=system_u:object_r:lspp_test_file_t

######################################################################
# common functions
######################################################################

function cleanup_context_policy {
	if semodule -l | grep -q test_context; then
		semodule -r test_context \
			|| exit_error "unable to unload policy prior to test"
	fi
}

function init_files {
	declare i
	rm -rf $testbase
	mkdir $testbase
	[[ $? != 0 ]] && exit_error "unable to create $testbase directory"
	for i in $filelist; do
		touch $testbase/$i
		[[ $? != 0 ]] && exit_error "unable to create file $testbase/$i"
	done
	for i in $dirlist; do
		mkdir $testbase/$i
		[[ $? != 0 ]] && exit_error "unable to create dir $testbase/$i"
	done
}

function load_test_policy {
	declare log_mark
	log_mark=$(stat -c %s $audit_log)
	semodule -i $1 
	[[ $? != 0 ]] && exit_error "unable to load test policy $1"
	augrok -q --seek=$log_mark type==MAC_POLICY_LOAD \
		subj=$subj auid=$auid success=yes \
		|| exit_fail "missing POLICY LOAD audit record"
}

# called as "verify_chcon_level oldlevel newlevel file"
function verify_chcon_level {
	declare log_mark
	log_mark=$(stat -c %s $audit_log)
	auditctl -a entry,always ${MODE:+-F arch=b$MODE} -S setxattr 
	auditctl -a entry,always ${MODE:+-F arch=b$MODE} -S getxattr 
	chcon -l $2 $3
	[[ $? != 0 ]] && exit_error "chcon failed"
	# chcon causes a setxattr and only shows the old context
	augrok -q --seek=$log_mark type==SYSCALL \
		subj=$subj auid=$auid success=yes comm=chcon \
		name=$3 obj=$obj_nolevel:$1 \
		|| exit_fail "missing audit record"
	log_mark=$(stat -c %s $audit_log)
	ls -lZ $3
	# ls causes a getxattr which shows the new context
	augrok -q --seek=$log_mark type==SYSCALL \
		subj=$subj auid=$auid success=yes comm=ls \
		name=$3 obj=$obj_nolevel:$2 \
		|| exit_fail "missing audit record"
	auditctl -D
}

# called as "verify_fail_chcon_level oldlevel newlevel file"
function verify_fail_chcon_level {
	declare log_mark
	log_mark=$(stat -c %s $audit_log)
	auditctl -a entry,always ${MODE:+-F arch=b$MODE} -S setxattr 
	auditctl -a entry,always ${MODE:+-F arch=b$MODE} -S getxattr 
	runcon -t lspp_test_generic_t -- chcon -l $2 $3
	[[ $? == 0 ]] && exit_fail "chcon successed unexpectedly"
	# chcon causes a setxattr and only shows the old context
	augrok -q --seek=$log_mark type==SYSCALL \
		subj=$gensubj auid=$auid success=no comm=chcon \
		name=$3 obj=$obj_nolevel:$1 \
		|| exit_fail "missing audit record"
	auditctl -D
}
