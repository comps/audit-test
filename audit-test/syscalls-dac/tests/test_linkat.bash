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

# if newpath (dir) is specified, create it, otherwise create
# one that will always allow file creation
if [ "$newpath" ]; then
    create_obj "$newpath"
    newpath="$OBJ_PATH"
else
    create_obj "dir#root:root#0777"
    newpath="$OBJ_PATH"
fi

eval_syscall "$res" "$errno" run_as -s "$subj" -c "$filecap" \
             do_linkat "AT_FDCWD" "$filename" "$newpath"/newlink

exit_pass

# vim: sts=4 sw=4 et :
