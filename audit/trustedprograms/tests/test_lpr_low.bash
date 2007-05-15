#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2007
#
#   This program is free software;  you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY;  without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
#   the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program;  if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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

append_cleanup delete_printer $PRINTER
create_parallel_printer $PRINTER $PRINTERDEV

append_cleanup chcon $SAVEDCON $PRINTERDEV
chcon -l $PRINTERCON $PRINTERDEV

# test
runcon $EXECCON -- /usr/bin/lpr -P $PRINTER /etc/passwd

msg_1="job=.* auid=$(</proc/self/loginuid) acct= obj=$EXECCON refused unable to access printer=$PRINTER.*failed."
augrok -q type=USER_LABELED_EXPORT msg_1=~"$msg_1" || exit_fail "missing: \"$msg_1\""

exit_pass

