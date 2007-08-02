#!/bin/bash
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

source testcase.bash || exit 2

######################################################################
# global variables
######################################################################

unset log_mark
unset auid subj subj_open subj_noopen
auid=$(</proc/self/loginuid)
subj=staff_u:lspp_test_r:lspp_harness_t:s0-s15:c0.c1023
subj_open=staff_u:lspp_test_r:test_open_t:s0
subj_noopen=staff_u:lspp_test_r:test_noopen_t:s0

######################################################################
# common functions
######################################################################

function cleanup_policy {
	semodule -l | grep -q policy_tools_test
	if [[ $? == 0 ]] 
	then
		semodule -r policy_tools_test
		[[ $? != 0 ]] && 
			exit_error "unable to unload policy prior to test"
	fi
}

function load_test_policy {
	log_mark=$(stat -c %s $audit_log)
	semodule -i $1 
	[[ $? != 0 ]] && exit_error "unable to load test policy $1"
	augrok --seek=$log_mark type==MAC_POLICY_LOAD \
		subj=$subj auid=$auid success=yes \
		|| exit_fail "missing POLICY LOAD audit record"
}

function load_test_policy_fail {
	log_mark=$(stat -c %s $audit_log)
	semodule -i $1 
	[[ $? == 0 ]] && exit_fail "incompatible test policy $1 loaded"
}

function update_test_policy {
	log_mark=$(stat -c %s $audit_log)
	semodule -u $1 
	[[ $? != 0 ]] && exit_error "unable to update test policy $1"
	augrok --seek=$log_mark type==MAC_POLICY_LOAD \
		subj=$subj auid=$auid success=yes \
		|| exit_fail "missing POLICY LOAD audit record for update"
}

function test_open_fail {
	log_mark=$(stat -c %s $audit_log)
	runcon -t $1 -l SystemLow cat $tmp1 >& /dev/null
	[[ $? == 0 ]] && exit_error "Read passed unexpectedly"
	augrok --seek=$log_mark type==SYSCALL \
		subj=$2 auid=$auid success=no \
		|| exit_fail "missing failed read audit record"
}

function test_open_pass {
	log_mark=$(stat -c %s $audit_log)
	runcon -t $1 -l SystemLow cat $tmp1 >& /dev/null
	[[ $? != 0 ]] && exit_error "Read passed unexpectedly"
	augrok --seek=$log_mark type==SYSCALL \
		subj=$2 auid=$auid success=yes \
		|| exit_fail "missing successful read audit record"
}
	
