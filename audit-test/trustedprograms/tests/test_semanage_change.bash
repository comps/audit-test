#!/bin/bash
###############################################################################
#   Copyright (c) 2016 Red Hat, Inc. All rights reserved.
#
#   This copyrighted material is made available to anyone wishing
#   to use, modify, copy, or redistribute it /bin/subject to the terms
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
# PURPOSE:
# Verify that changes policy object store for node, interface, port
# and fcontext objects are audited.
#
# Operations that cause changes, and which are checked by this test are:
#  - add (--add)
#  - delete (--delete)
#  - delete all (--deleteall)
#  - modify (--modify)

source testcase.bash || exit 2

set -x

# the test cases will be defined in an array of strings, where
# - each pair of strings represents a test case
# - first string represents the semanage command with parameters
# - the second string defines the expected msg_1 part of the audit event
#   - multiple events can be specified by using the '|' as the delimiter
#   - when event is an empty string, the semanage command is expected to fail

cases_port=(
    "port --add -t ssh_port_t -p tcp 422" "resrc=port op=add lport=422 proto=6 tcontext=system_u:object_r:ssh_port_t:s0"
    "port --add -t http_port_t -p udp 422" "resrc=port op=add lport=422 proto=17 tcontext=system_u:object_r:http_port_t:s0"
    "port --modify -t http_port_t -p tcp 422" "resrc=port op=modify lport=422 proto=6 tcontext=system_u:object_r:http_port_t:s0"
    "port --delete -p udp 422" "resrc=port op=delete lport=422 proto=17"
    "port --add -t ssh_port_t -p udp 422" "resrc=port op=add lport=422 proto=17 tcontext=system_u:object_r:ssh_port_t:s0"
    "port -D" "resrc=port op=delete lport=422 proto=6|resrc=port op=delete lport=422 proto=17"
)

# modify action is not tested as the default policy contains only one node type (node_t)
cases_node=(
    "node --add -t node_t -p ipv4 -M 255.255.255.0 127.0.0.1" "resrc=node op=add laddr=127.0.0.1 netmask=255.255.255.0 proto=4 tcontext=system_u:object_r:node_t:s0"
    "node --add -t node_t -p ipv4 -M 255.255.255.0 127.0.0.2" "resrc=node op=add laddr=127.0.0.2 netmask=255.255.255.0 proto=4 tcontext=system_u:object_r:node_t:s0"
    "node --add -t node_t -p ipv6 -M ::1 ::1" "resrc=node op=add laddr=::1 netmask=::1 proto=41 tcontext=system_u:object_r:node_t:s0"
    "node --add -t node_t -p ipv6 -M ::1 ::2" "resrc=node op=add laddr=::2 netmask=::1 proto=41 tcontext=system_u:object_r:node_t:s0"
    "node --delete -t node_t -p ipv4 -M 255.255.255.0 127.0.0.1" "resrc=node op=delete laddr=127.0.0.1 netmask=255.255.255.0 proto=4"
    "node --delete -t node_t -p ipv6 -M ::1 ::2" "resrc=node op=delete laddr=::2 netmask=::1 proto=41"
    "node -D" "resrc=node op=delete laddr=127.0.0.2 netmask=255.255.255.0 proto=4|resrc=node op=delete laddr=::1 netmask=::1 proto=41"
)

cases_interface=(
    "interface --add -t netif_t -r s0 testif1" "resrc=interface op=add netif=testif1 tcontext=system_u:object_r:netif_t:s0"
    "interface --add -t netif_t -r s15 testif2" "resrc=interface op=add netif=testif2 tcontext=system_u:object_r:netif_t:s15"
    "interface --delete testif1" "resrc=interface op=delete netif=testif1"
    "interface --modify -t netif_t -r s2 testif2" "resrc=interface op=modify netif=testif2 tcontext=system_u:object_r:netif_t:s2"
    "interface --add -t netif_t -r s1 testif1" "resrc=interface op=add netif=testif1 tcontext=system_u:object_r:netif_t:s1"
    "interface -D" "resrc=interface op=delete netif=testif1|resrc=interface op=delete netif=testif2"
)

# we won't be testing -D option, so we do not loose local modifications from evaluated configuration
cases_fcontext=(
    "fcontext --add -t var_t -s staff_u -r s15 -f s /somefile2" "resrc=fcontext op=add tglob=\"/somefile2\" ftype=socket tcontext=staff_u:object_r:var_t:s15"
    "fcontext --modify -t bin_t -s user_u -r s0 -f s /somefile2" "resrc=fcontext op=modify tglob=\"/somefile2\" ftype=socket tcontext=user_u:object_r:bin_t:s0"
    "fcontext --delete -f s /somefile2" "resrc=fcontext op=delete tglob=\"/somefile2\" ftype=socket"
    "fcontext --add -t bin_t -s user_u -r s2 /somefile1" "resrc=fcontext op=add tglob=\"/somefile1\" ftype=any tcontext=user_u:object_r:bin_t:s2"
    "fcontext --delete /somefile1" "resrc=fcontext op=delete tglob=\"/somefile1\" ftype=any"
    "fcontext --add -t bin_t -s user_u -r s2 -f f /somefile1" "resrc=fcontext op=add tglob=\"/somefile1\" ftype=file tcontext=user_u:object_r:bin_t:s2"
    "fcontext --delete /somefile1 -f f" "resrc=fcontext op=delete tglob=\"/somefile1\" ftype=file"
    "fcontext --add -t bin_t -s user_u -r s2 -f d /somefile1" "resrc=fcontext op=add tglob=\"/somefile1\" ftype=dir tcontext=user_u:object_r:bin_t:s2"
    "fcontext --delete /somefile1 -f d" "resrc=fcontext op=delete tglob=\"/somefile1\" ftype=dir"
    "fcontext --add -t bin_t -s user_u -r s2 -f c /somefile1" "resrc=fcontext op=add tglob=\"/somefile1\" ftype=char tcontext=user_u:object_r:bin_t:s2"
    "fcontext --delete /somefile1 -f c" "resrc=fcontext op=delete tglob=\"/somefile1\" ftype=char"
    "fcontext --add -t bin_t -s user_u -r s2 -f b /somefile1" "resrc=fcontext op=add tglob=\"/somefile1\" ftype=block tcontext=user_u:object_r:bin_t:s2"
    "fcontext --delete /somefile1 -f b" "resrc=fcontext op=delete tglob=\"/somefile1\" ftype=block"
    "fcontext --add -t bin_t -s user_u -r s2 -f l /somefile1" "resrc=fcontext op=add tglob=\"/somefile1\" ftype=symlink tcontext=user_u:object_r:bin_t:s2"
    "fcontext --delete /somefile1 -f l" "resrc=fcontext op=delete tglob=\"/somefile1\" ftype=symlink"
    "fcontext --add -t bin_t -s user_u -r s2 -f p /somefile1" "resrc=fcontext op=add tglob=\"/somefile1\" ftype=pipe tcontext=user_u:object_r:bin_t:s2"
    "fcontext --delete /somefile1 -f p" "resrc=fcontext op=delete tglob=\"/somefile1\" ftype=pipe"
)

# we won't be testing --modify action as it calls --add in the code
cases_fcontext_equal=(
    "fcontext --add -e /var /testvar" "resrc=fcontext op=add-equal sglob=\"/testvar\" tglob=\"/var\""
    "fcontext --delete /testvar" "resrc=fcontext op=delete-equal tglob=\"/testvar\""
)


# sanity checks
[ -z "$1" ] && exit_error "no object to test"
eval cases=(\"\${cases_${1}[@]}\")
[ ${#cases[@]} -eq 0 ] && exit_error "no cases defined for $1"

# specific setup/cleanup
case "$1" in
    interface)
        # add dummy network devices
        modprobe dummy
        prepend_cleanup "rmmod dummy"
        ip link add testif1 type dummy
        prepend_cleanup "ip link del testif1"
        ip link add testif2 type dummy
        prepend_cleanup "ip link del testif2"
        prepend_cleanup "semanage interface -D"
        ;;
    port|node)
        prepend_cleanup "semanage $1 -D"
        ;;
    fcontext)
        prepend_cleanup "semanage fcontext -d -f s /somefile2"
        for file_type in a f d c b l p; do
            prepend_cleanup "semanage fcontext -d -f $file_type /somefile1"
        done
        ;;
    fcontext_equal)
        prepend_cleanup "semanage fcontext --delete /var /testvar"
        ;;
esac

# do the actual testing
for ((i=0;i<${#cases[@]};i+=2)); do
    audit_mark=$(get_audit_mark)
    cmd=${cases[i]}
    event=${cases[i+1]}

    # do the operation and check if successfule
    semanage $cmd; ret=$?

    # pass scenario
    [ -n "$event" -a $ret -ne 0 ] && exit_fail "semanage $cmd failed unexpectedly"

    # fail scenario
    [ -z "$event" -a $ret -eq 0 ] && exit_fail "semanage $cmd passed unexpectedly"

    if [ -n "$event" ]; then
        # be verbose about the changes
        augrok --seek=$audit_mark type=USER_MAC_CONFIG_CHANGE
    else
        # unexpected event
        augrok --seek=$audit_mark type=USER_MAC_CONFIG_CHANGE && \
            exit_fail "unexpected USER_MAC_CONFIG_CHANGE event"
    fi

    # check for correct audit record
    IFS='|'
    for value in $event; do
        msg="$value comm=\"semanage\".*res=success"
        augrok --seek=$audit_mark type=USER_MAC_CONFIG_CHANGE msg_1=~"$msg" || \
            exit_fail "could not find USER_MAC_CONFIG_CHANGE audit record with '$value'"
    done
    unset IFS
done

exit_pass
