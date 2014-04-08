#!/bin/bash
###############################################################################
#   Copyright (c) 2014 Red Hat, Inc. All rights reserved.
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
# DAC tests for mq_open syscall
#
# Author: Miroslav Vadkerti <mvadkert@redhat.com>
#

source syscalls_dac_functions.bash || exit 2

create_subj "$subj"

# expects always mqdir type
[ "$objdir" ] && create_obj "$objdir"

create_obj "$obj"
filename="$OBJ_PATH"

eval_syscall "$res" "$errno" run_as -s "$subj" -c "$subj_dropcap" \
             do_mq_open "$filename" "$op"

exit_pass

# vim: sts=4 sw=4 et :
