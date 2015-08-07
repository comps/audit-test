###############################################################################
#   Copyright (c) 2015 Red Hat, Inc. All rights reserved.
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

source functions.bash || exit 2

# ipsec_local_conf - create/overwrite a local config (for this machine)
function ipsec_local_conf {
    cat > /etc/ipsec.d/audit-test-local.conf
}
# ipsec_local_secrets - create/overwrite a local secrets file
function ipsec_local_secrets {
    cat > /etc/ipsec.d/audit-test-local.secrets
}
# conf/secrets cleanup
function ipsec_local_cleanup {
    rm -f /etc/ipsec.d/audit-test-local.*
}

# ipsec_addr_local - get ip addr of this machine when viewed from NS
function ipsec_addr_local {
    local ipv="$1"
    send_ns ${ipv:+-$ipv} remote 'myaddr'
}
# ipsec_addr_remote - get ip addr of NS
function ipsec_addr_remote {
    local ipv="$1"
    [ "$ipv" ] || ipv=4
    host_ns "-$ipv" remote
}

# ipsec_std_header - get example/standard config header
function ipsec_std_header {
cat <<EOF
config setup
	protostack=netkey
	nat_traversal=yes
	plutostderrlog=/var/log/pluto.log

EOF
}

# vim: sts=4 sw=4 et :
