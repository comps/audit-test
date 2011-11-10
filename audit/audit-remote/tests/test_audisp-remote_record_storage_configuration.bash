#!/bin/bash
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
# Test storage location of remotely generated audit records is configurable.
# Related SFR: FMT_MTD.1(AS)
#
# AUTHOR: Eduard Benes <ebenes@redhat.com>
#
# DESCRIPTION:
#   Using basic connection test that storage of remotely generated records
#   coming from audisp-remote plugin is configurable via auditd.conf.
#   TOE acting as server and client. Client uses port 61 and server listens
#   on port 60.
#
#


source audisp-remote_functions.bash || exit 2

#
# Configure
#

common_server_startup || exit_error "common_server_startup"
configure_local_audit_server
configure_local_audisp_remote

#
# Test
#

# Be verbose
set -x

# Use default storage location
test_msg="`create_user_test_msg`"
send_audisp_remote_test_msg $test_msg
check_received_test_msg

# Modify storage location and check arrived records
new_log="/var/log/audit/new-audit.log"
stop_auditd
append_cleanup rm -f $new_log
write_config -s $auditd_conf log_file=$new_log
cat $auditd_conf
start_auditd $new_log || exit_error "Failed to start auditd service"
send_audisp_remote_test_msg $test_msg
check_received_test_msg $new_log

# If we got this far, we are good to PASS
exit_pass
