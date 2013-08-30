#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
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

source testcase.bash || exit 2

######################################################################
# global variables
######################################################################

if [[ "$DISTRO" != "RHEL" ]] ; then
    if [ -f /etc/vsftpd/vsftpd.conf ]; then
        vsftpd_conf=/etc/vsftpd/vsftpd.conf
    elif [ -f /etc/vsftpd.conf ]; then
        vsftpd_conf=/etc/vsftpd.conf
    else
        exit_error "Unable to find vsftpd.conf"
    fi

    vsftpd_init=/etc/init.d/vsftpd
fi

# XXX should determine this from policy
login_context=staff_u:lspp_test_r:local_login_t:s0-s15:c0.c1023

function user_cleanup {
        semanage login -d $TEST_USER
}

set -x
