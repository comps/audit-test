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
# FMT_MTD.1(AUD-AE) - Query or modify audited event set
#
# AUTHOR: Eduard Benes <ebenes@redhat.com>
#
# DESCRIPTION:
#   Test that TSF restricts the ability to query, modify the set of events 
#   audited by a remote trusted IT system to processes with the capability
#   CAP_AUDIT_CONTROL.
#

source testcase.bash || exit 2


#
# Configure
#

# Make sure we don't leave a rule after permature exit
prepend_cleanup "/sbin/auditctl -D"

#
# Test
#

# Be verbose
set -x

# Show what capabilities do we have
getpcaps $$

#NOTE: In MLS mode you have to have SELinux role auditadm_r to use auditctl
# lspp_test_r which should be used for running tests has all reguired rules already.
#============= auditadm_t ==============
#allow auditadm_t self:capability setpcap;

# Make sure we have the ability to query, modify the set of events audited by
# a remote trusted IT system to processes with the capability.
# Query
/sbin/auditctl -l || exit_error "Should be able to query"
# Modify
/sbin/auditctl -a exit,always -F subj_clr=s0:c36,c446 || exit_error "Modify/add"
/sbin/auditctl -l
/sbin/auditctl -d exit,always -F subj_clr=s0:c36,c446 || exit_error "Modify/del"
/sbin/auditctl -l
/sbin/auditctl -D || exit_error "Modify/delall"

# Make sure we don't have the capability to query or modify the set of audited
# events after dropping CAP_AUDIT_CONTROL.
capsh --drop=cap_dac_override,cap_audit_control -- -c 'getpcaps $$' 2>&1 | \
  grep cap_dac_override,cap_audit_control || exit_error "Failed to drop CAP_..."

# Now we shoudl be NOT allowed to query or modify rules:
# Query
capsh --drop=cap_dac_override,cap_audit_control -- -c '/sbin/auditctl -l' 2>&1 | \
  grep "(Operation not permitted)" || exit_fail "Should fail to query"
# Modify
capsh --drop=cap_dac_override,cap_audit_control -- -c '/sbin/auditctl -D' 2>&1 | \
  grep "(Operation not permitted)" || exit_fail "Should fail to modify (-D)"
capsh --drop=cap_dac_override,cap_audit_control -- \
  -c '/sbin/auditctl -a exit,always -F subj_clr=s0:c36,c446' 2>&1 | \
  grep "(Operation not permitted)" || exit_fail "Should fail to modify (-a)"
capsh --drop=cap_dac_override,cap_audit_control -- \
  -c '/sbin/auditctl -d exit,always -F subj_clr=s0:c36,c446' 2>&1 | \
  grep "(Operation not permitted)" || exit_fail "Should fail to modify (-d)"

# If we got this far, we are good to PASS
exit_pass

