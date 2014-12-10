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

source containers_functions.bash || exit 2

#
# this test verifies that Docker created cgroups for the testing container,
# enabling resource isolation
#

cgroup="$1"

cgroup_root="/sys/fs/cgroup/$cgroup"
[ -e "$cgroup_root" ] || exit_error "$cgroup_root doesn't exist"

container_id="$DOCKER_CONTAINER"
[ "$container_id" ] || exit_error "could not get docker container id"
container_init="$DOCKER_INITPID"
[ "$container_init" ] || exit_error "could not get docker container init pid"

# test: docker-created cgroup for the container needs to exist
container_cgroup="$cgroup_root/system.slice/docker-${container_id}.scope"
[ -d "$container_cgroup" ] || exit_fail "container cgroup not found"

# test: the cgroup needs to have pid of the container init process
grep "$container_init" "$container_cgroup"/tasks
[ $? -eq 0 ] || exit_fail "container init pid not found in container cgroup"

exit_pass

# vim: sts=4 sw=4 et :
