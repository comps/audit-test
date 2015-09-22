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
# Verify the audit record and labeled output of a postscript print job
# embedded in this test is a simple postscript file and the expected
# postscript output.  The test creates a socket printer and prints the file,
# capturing the output with netcat.  The test checks that the ouput matches
# and that the job was audited.


source tp_print_functions.bash || exit 2

# setup

# simple file to be printed
INFILE=lpr_ps_input.ps

# labeled output only matches correctly labeled output with:
# printer of tests 
# job-id  of 42
# user    of root
# file    of lpr_ps_input.ps
EXPECTED=lpr_ps_expected.ps

backup $INFILE $EXPECTED

EXECCON=staff_u:lspp_test_r:lspp_harness_t:SystemLow-SystemHigh
FILECON=staff_u:object_r:sysadm_home_t:SystemLow
JOBNO=42
OUTFILE=lpr_ps_output.ps
PRINTER=tests

setup_cupsd
create_socket_printer $PRINTER

# make sure next job number is known
set_next_jobid $JOBNO

chcon $FILECON $INFILE
prepend_cleanup delete_printer $PRINTER
prepend_cleanup "rm -f $OUTFILE"

# test
runcon $EXECCON /usr/bin/lpr -P $PRINTER $INFILE
create_socket_listener $OUTFILE

# verify
msg_1="job=.* auid=$(</proc/self/loginuid) acct=root printer=$PRINTER title=$INFILE obj=$EXECCON label=SystemLow-SystemHigh"

augrok -q type=USER_LABELED_EXPORT msg_1=~"$msg_1" || exit_fail "missing: \"$msg_1\""

if [ \! -f $OUTFILE ]; then
    exit_fail "File not found: $OUTFILE"
else
    sed -i \
	-e s/%%CreationDate.*$// \
	-e s/%%Creator.*$// \
	-e s/%%For.*$// \
	-e s/%%Title.*$// \
	$OUTFILE

    sed -i \
	-e s/%%CreationDate.*$// \
	-e s/%%Creator.*$// \
	-e s/%%For.*$// \
	-e s/%%Title.*$// \
	$EXPECTED

    diff $OUTFILE $EXPECTED || exit_fail "Labeled output does not match"
fi

# cleanup 

exit_pass

