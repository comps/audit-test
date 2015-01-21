#!/bin/bash
###############################################################################
#   Copyright (c) 2014 Red Hat, Inc. All rights reserved.
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
# AUTHOR: Miroslav Vadkerti <mvadkert@redhat.com>
#
# DESCRIPTION:
# This test implements miscellaneous related sssd testcases.
#
# Testcases:
#  unprivileged - tests if sssd runs as a unprivileged user and main
#                 sssd process runs as root
#

source testcase.bash || exit 2

# globals
SSSD_CONF="/etc/sssd/sssd.conf"

# in MLS mode expect also pam_namespace in grantors field
[ "$PPROFILE" = "lspp" ] && PAM_ADDTL="pam_namespace," || PAM_ADDTL=

# be verbose
set -x

###############################################################################
## unprivileged
###############################################################################

function unprivileged {
    # check if sssd correctly configured
    grep "user = sssd" $SSSD_CONF || exit_fail \
        "Required 'user = sssd' configuration not founf in $SSSD_CONF"

    # check if main sssd process is running as root
    [ "$(ps h -C sssd -o user)" == "root" ] || exit_fail \
        "The main sssd process is not running as root"

    # check if all sssd services are running with expected unprivileged user
    for SRV in sssd_be sssd_nss sssd_pam sssd_ssh sssd_pac; do
        [ "$(ps h -C $SRV -o user)" == "sssd" ] || exit_fail \
            "The $SRV process is not running as unprivileged sssd user"
    done
}

# enable sssd
restart_service sssd
prepend_cleanup "stop_service sssd"

# choose test
if [ "$(type -t $1)" = "function" ]; then
    eval $1
else
    exit_error "Unknown test or no test given"
fi

exit_pass

# vim: ts=4 sw=4 sts=4 ft=sh et ai:
