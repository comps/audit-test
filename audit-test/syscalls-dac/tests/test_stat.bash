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
# DAC tests for stat syscall
#
# Author: Ondrej Moris <omoris@redhat.com>
#

source syscalls_dac_functions.bash || exit 2

create_subj "$subj"

if [ "$objdir" ]; then
    create_obj "$objdir"
    filename="$OBJ_PATH"/testfile
    create_obj "$obj" "$filename"
else
    create_obj "$obj"
    filename="$OBJ_PATH"
fi

eval_syscall "$res" "$errno" run_as -s "$subj" -c "$filecap" \
             do_stat "$filename"

exit_pass

# vim: sts=4 sw=4 et :
