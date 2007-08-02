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
# Verify cupsd only exposes job which a user is allowed to see
# Print two jobs, one as a user @ SystemLow and one @ Secret then show that
# only the user:SystemLow job is shown to a user lpq at SystemLow and that
# both jobs are shown to sysadm:SystemLow since they have mls_read_up

source tp_print_functions.bash || exit 2

# setup

LPR1CON=staff_u:lspp_test_r:lspp_test_generic_t:SystemLow-SystemLow
LPR2CON=staff_u:lspp_test_r:lspp_test_generic_t:Secret-Secret

LPQ1CON=staff_u:lspp_test_r:lspp_test_generic_t:SystemLow

CON1OUT=lspp_test_generic_lpq.out
CON2OUT=sysadm_lpq.out

printer=tests$$

prepend_cleanup delete_printer $printer
prepend_cleanup semanage login -d $TEST_USER
create_socket_printer $printer
cupsdisable $printer

runcon $LPR1CON -- lpr -P $printer -T staff-syslow /etc/passwd
runcon $LPR2CON -- lpr -P $printer -T staff-secret /etc/passwd

# calling login in this manner leaves an entry in /var/run/utmp
# use backup (and automatic restore) to work around this
backup /var/run/utmp

semanage login -a -s staff_u -r SystemLow-SystemHigh $TEST_USER || \
  exit_error "unable to set $TEST_USER to staff_u"

# test
prepend_cleanup rm -f $CON1OUT $CON2OUT
runcon $LPQ1CON -- lpq -P $printer > $CON1OUT
(
  export printer
  export CON2OUT
  expect -c '
    log_file -noappend $env(CON2OUT)
    spawn login
    expect -nocase {login: $} {send "$env(TEST_USER)\r"}
    expect -nocase {password: $} {send "$env(TEST_USER_PASSWD)\r"}
    expect -nocase {level} {send "N\r"}
    send "PS1=\"::\\#$ \"\r";
    expect {
      ::2 { send "lpq -P $env(printer)\r"; exp_continue; }
      ::3 { exit 0; }
    }
    exit 1; '
)

if [ $? -ne 0 ]; then
  exit_error "sysadm lpq expect did not finish properly"
fi

if [ \! -f $CON2OUT ]; then
  exit_error "sysadm lpq output was not generated"
fi

# verify
if [ -n "$(grep staff-secret $CON1OUT)" ]; then
  exit_fail "staff-secret job appeared in staff_t lpq output"
fi
grep -q staff-syslow $CON1OUT || \
  exit_fail "staff-syslow did not appear in staff lpq output"

grep -q staff-syslow $CON2OUT || \
  exit_fail "staff-syslow did not appear in sysadm lpq output"
grep -q staff-secret $CON2OUT || \
  exit_fail "staff-secret did not appear in sysadm lpq output"

exit_pass
