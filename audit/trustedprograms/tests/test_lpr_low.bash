#!/bin/bash
###############################################################################
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
###############################################################################
# 
# PURPOSE:
# Verify the audit of an export failure when a user attempts to print above 
# their level.  A parallel printer is created and set to Secret, then a user
# attempts to print to this printer from a SystemLow-Unclassified context.
# the job is rejected by cupsd and the test verifies that it is audited.

source tp_print_functions.bash || exit 2

# setup
PRINTER=testp$$
PRINTERDEV=/dev/parport0
EXECCON=staff_u:lspp_test_r:lspp_test_generic_t:SystemLow-Unclassified
PRINTERCON=Secret
SAVEDCON=`ls -lZ $PRINTERDEV | awk '{print $4}'`

prepend_cleanup delete_printer $PRINTER
create_parallel_printer $PRINTER $PRINTERDEV

prepend_cleanup chcon $SAVEDCON $PRINTERDEV
chcon -l $PRINTERCON $PRINTERDEV

# test
runcon $EXECCON -- /usr/bin/lpr -P $PRINTER /etc/passwd

msg_1="job=.* auid=$(</proc/self/loginuid) acct= obj=$EXECCON refused unable to access printer=$PRINTER.*failed."
augrok -q type=USER_LABELED_EXPORT msg_1=~"$msg_1" || exit_fail "missing: \"$msg_1\""

exit_pass

