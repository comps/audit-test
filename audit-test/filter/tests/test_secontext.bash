#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2011
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
###############################################################################
#
# PURPOSE:
# Test the ability to filter on SELinux labels and user role

source filter_functions.bash || exit 2

# setup
user_auid=$(cat /proc/self/loginuid)

# verify no record by default
# audit log marker
log_mark=$(stat -c %s $audit_log)

# generate an audit event
do_open $tmp1 read

augrok --seek=$log_mark "name==$tmp1" "auid==$user_auid" \
    && exit_error "Unexpected record found."

# Need untranslated labels
service mcstrans stop 63>/dev/null
prepend_cleanup "service mcstrans start 63>/dev/null"

declare subj_level obj_level user_role target
declare aurule grok

target=$tmp1

case $1 in
	subj_sen)
		subj_level=$(id -Z | awk -F : '{print $4}'| awk -F - '{print $1}')
		aurule="subj_sen=$subj_level"
		grok="subj=~$subj_level"
		;;
	subj_clr)
		subj_level=$(id -Z | awk -F : '{print $4 ":" $5}'| awk -F - '{print $2}')
		aurule="subj_clr=$subj_level"
		grok="subj=~$subj_level"
		;;
	subj_role)
		user_role=$(id -Z | awk -F : '{print $2}')
		aurule="subj_role=$user_role"
		grok="subj=~$user_role"
		;;
	obj_lev_low)
		obj_level=$(ls -Z $tmp1 | awk -F : '{print $4}' | awk '{print $1}')
		aurule="obj_lev_low=$obj_level"
		grok="obj=~$obj_level"
		;;
	obj_lev_high_base)
		obj_level=$(ls -Z $tmp1 | awk -F : '{print $4 ":" $5}' | awk '{print $1}')
		aurule="obj_lev_high=$obj_level"
		grok="obj=~$obj_level"
		;;
	obj_lev_high_mls)
		target=/tmp		# need a ranged obj
		obj_level=$(ls -Zd /tmp | awk -F : '{print $4 ":" $5}' | awk '{print $1}' | awk -F - '{print $2}')
		aurule="obj_lev_high=$obj_level"
		grok="obj=~$obj_level"
		;;
	*) exit_error "test must specify valid context component"
esac

auditctl -a exit,always ${MODE:+-F arch=b$MODE} -S open -F $aurule
prepend_cleanup "auditctl -d exit,always ${MODE:+-F arch=b$MODE} -S open -F $aurule"

# audit log marker
log_mark=$(stat -c %s $audit_log)

# generate an audit event
do_open $target read

augrok --seek=$log_mark "name==$target" "$grok" \
    || exit_fail "Expected record not found."

exit_pass
