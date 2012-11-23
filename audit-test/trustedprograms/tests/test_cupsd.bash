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
# Verify audit messages from cupsd startup
# This test ensures that cupsd is started with options:
# Classification mls
# ClassifyOverride yes
# PerPageLabels no
# and then verifies that the audits of these settings appear in the audit log.

source tp_print_functions.bash || exit 2

# setup
setup_cupsd
backup /etc/cups/cupsd.conf

sed -ie "s/Classification.*/Classification mls/" /etc/cups/cupsd.conf
sed -ie "s/.*ClassifyOverride.*/ClassifyOverride yes/" /etc/cups/cupsd.conf
sed -ie "s/.*PerPageLabels.*/PerPageLabels no/" /etc/cups/cupsd.conf

# test
expect -c "
    spawn /usr/sbin/run_init /etc/init.d/cups restart
    expect {
        -nocase \"password: \" {send \"$PASSWD\\r\"; exp_continue}
        eof
    }"

# verify
augrok -q type=LABEL_LEVEL_CHANGE msg_1=~".Config. Classification=mls.*" || exit_fail
augrok -q type=USYS_CONFIG msg_1=~".Config. ClassifyOverride=enabled.*" || exit_fail
augrok -q type=USYS_CONFIG msg_1=~".Config. PerPageLabels=disabled.*" || exit_fail

# cleanup

exit_pass

