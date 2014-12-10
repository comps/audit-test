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
# this test verifies that newly created Docker containers have their own
# Linux namespace instances and don't share namespaces with the rest of
# the system (init ns)
#
# before doing that, it needs to make sure that the system itself is in
# a known sane state - if, for example, all newly spawned daemons (by the init
# daemon) get their own sets of namespaces, then the containers will have ns IDs
# different from pid 1 (init ns), but no thanks to Docker isolation
#

ns="$1"

# sanity: check that pid 1, the docker daemon and this shell share the same ns
init_ns=$(readlink "/proc/1/ns/$ns")
[ "$init_ns" ] || exit_error "could not get $ns id for pid 1"
docker_pid=$(pgrep -P 1 -x 'docker')  # daemon, parent pid 1
docker_ns=$(readlink "/proc/$docker_pid/ns/$ns")
[ "$docker_ns" ] || exit_error "could not get $ns id for docker"
our_ns=$(readlink "/proc/self/ns/$ns")
[ "$our_ns" ] || exit_error "could not get $ns id for current shell"

[ "$init_ns" = "$docker_ns" -a "$init_ns" = "$our_ns" ] || \
    exit_error "sanity failed: init/docker/self have different $ns ids"

# test: check whether the container namespace id differs from init ns
container_pid="$DOCKER_INITPID"
container_ns=$(readlink "/proc/$container_pid/ns/$ns")
[ "$container_ns" ] || exit_error "could not get $ns id of container init"

[ "$container_ns" = "$init_ns" ] && \
    exit_fail "container $ns id is the same as init ns, isolation failed"

exit_pass

# vim: sts=4 sw=4 et :
