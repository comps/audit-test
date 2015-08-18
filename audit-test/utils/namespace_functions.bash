###############################################################################
#   Copyright (c) 2015 Red Hat, Inc. All rights reserved.
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

PATH="$TOPDIR/utils/namespaces:$PATH"

#
# functions useful for linux namespace testing
#
# these work on namespace "sets" using a dummy processes to "hold" the sets
# - a set is then defined as all namespaces of a given PID
#
# example:
#   $ local pid=
#   $ pid=$(create_nsset mnt ipc net) || error "creating nsset failed"
#   $ exec_nsset $pid cat /some/file

# unshare a set of namespaces, storing it in a dummy process (returning its pid)
create_nsset()
{
    clonens "$@"
}

# execute given command in a namespace set defined a pid ($1)
exec_nsset()
{
    local pid="$1"
    shift
    setns -c -p "$pid" "$@"
}

# vim: sts=4 sw=4 et :
