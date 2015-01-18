###############################################################################
#   Copyright (c) 2011, 2014 Red Hat, Inc. All rights reserved.
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
# AUTHOR: Ondrej Moris <omoris@redhat.com>
#
# DESCRIPTION:
# Helper functions for ipsec testing used in trustedprograms bucket.

source functions.bash || exit 2

######################################################################
# global variables
######################################################################

unset ipsec_nc
unset ipsec_src
unset ipsec_dst
unset ipsec_dst_port
unset ipsec_log_mark
unset ipsec_conf
unset remote_script

# IPsec configuration.
ipsec_conf="/etc/ipsec.conf"

# IPsec secrets.
ipsec_secrets="/etc/ipsec.secrets"

# Libreswan log file.
pluto_log="/var/log/pluto.log"

######################################################################
# functions
######################################################################

# normalize_addr - Add leading zeros to a comparessed IPv6 address
#
# INPUT
# IPv6 address
#
# OUTPUT
# Writes the normalized IPv6 address to stdout
#
# DESCRIPTION
# This function add leading zeros to a compressed IPv6 address,
# e.g. 2620:52:0:2223:216:3eff:fe00:28 will be updated to
# 2620:52:0000:2223:0216:3eff:fe00:0028
#
function ipsec_normalize_ip {
    addr=$1
    if [[ $ipsec_ip_ver == "6" ]]; then
        while [ $[`echo $addr | sed 's/[^:]//g' | wc -m`-1] -lt 7 ]; do
            addr=`echo $addr | sed 's/::/:0000::/g'`
        done

        for ((loop_cnt=0; loop_cnt<8; loop_cnt++)); do
            addr=$(echo $addr | sed -e 's/:\([0-9a-f]\):/:000\1:/g' \
                -e 's/:\([0-9a-f][0-9a-f]\):/:00\1:/g' \
                -e 's/:\([0-9a-f][0-9a-f][0-9a-f]\):/:0\1:/g' \
                -e 's/:\([0-9a-f]\)$/:000\1/g' \
                -e 's/:\([0-9a-f][0-9a-f]\)$/:00\1/g' \
                -e 's/:\([0-9a-f][0-9a-f][0-9a-f]\)$/:0\1/g' \
                -e 's/::/:0000:/g' \
                -e 's/^:/0000:/g' \
                -e 's/:$/:0000/g')
        done
    fi
    echo $addr
}

# ipsec_init - Initialize global variables
#
# INPUT
# IP version
#
# OUTPUT
# None
#
# DESCRIPTION
# This function initializes following global variables:
#
#  * ipsec_ip_version (IP version),
#  * ipsec_src (IP address of TOE),
#  * ipsec_dst (IP address of NS),
#  * ipsec_dst_port (lblnet_tst_server port on NS).
#
function ipsec_init {

    ipsec_ip_ver=$1
    if [ "$ipsec_ip_ver" == "6" ]; then
        [[ -n $(eval echo \$LBLNET_SVR_IPV6) ]] || exit_error
        ipsec_src=$LOCAL_IPV6
        ipsec_dst=$LBLNET_SVR_IPV6
        ipsec_dst_port=4000
    elif [ "$ipsec_ip_ver" == "4" ]; then
        [[ -n $(eval echo \$LBLNET_SVR_IPV4) ]] || exit_error
        ipsec_src=$LOCAL_IPV4
        ipsec_dst=$LBLNET_SVR_IPV4
        ipsec_dst_port=4201
    else
        exit_error "Unexpected IP version (4 or 6 expected)"
    fi

    normalized_ipsec_src=$(ipsec_normalize_ip $(echo $ipsec_src))
    normalized_ipsec_dst=$(ipsec_normalize_ip $(echo $ipsec_dst))

    ipsec_log_mark=$(stat -c %s $audit_log)
    audit_mark="$(date "+%T")"
    ipsec_nc="nc -${ipsec_ip_ver} -w 30 -v "

    sleep 5
}

# ipsec_backup_conf
#
# INPUT
# None
#
# OUTPUT
# None
#
# DESCRIPTION
# This function backups and reset IPsec configuration file ($ipsec_conf).
#
function ipsec_backup_conf {

    cp $ipsec_conf ${ipsec_conf}.backup

    echo "version 2.0"                               >  $ipsec_conf
    echo ""                                          >> $ipsec_conf
    echo "config setup"                              >> $ipsec_conf
    echo "        protostack=netkey"                 >> $ipsec_conf
    echo "        nat_traversal=yes"                 >> $ipsec_conf
    echo "        plutostderrlog=/var/log/pluto.log" >> $ipsec_conf
    echo ""                                          >> $ipsec_conf
}

# ipsec_restore_conf
#
# INPUT
# None
#
# OUTPUT
# None
#
# DESCRIPTION
# This function restores IPsec configuration file ($ipsec_conf).
#
function ipsec_restore_conf {

    cp  ${ipsec_conf}.backup $ipsec_conf
}

# ipsec_backup_secrets
#
# INPUT
# None
#
# OUTPUT
# None
#
# DESCRIPTION
# This function backups IPsec secrets file ($ipsec_secrets).
#
function ipsec_backup_secrets {

    cp $ipsec_secrets ${ipsec_secrets}.backup
}

# ipsec_set_secrets
#
# INPUT
# Authentication type (PSK or RSA)
# Secret (for PSK) or private key/token (for RSA)
#
# OUTPUT
# None
#
# DESCRIPTION
# This function removes connection of given name from IPsec
# configuration file ($ipsec_conf).
#
function ipsec_set_secrets {

    echo ": $1 $2" > $ipsec_secrets
}

# ipsec_restore_secrets
#
# INPUT
# None
#
# OUTPUT
# None
#
# DESCRIPTION
# This function restores IPsec secrets file ($ipsec_secrets).
#
function ipsec_restore_secrets {

    cp  ${ipsec_secrets}.backup $ipsec_secrets
}

# ipsec_add_connection
#
# INPUT
# * Connection name
# * Connection specification
#
# OUTPUT
# None
#
# DESCRIPTION
# This function adds a connection of given name and specification
# into IPsec configuration file ($ipsec_conf). Specification
# format is as follows: "key1=value1|key2=value|...|".
#
function ipsec_add_connection {

    local conn=$1
    shift

    [ -z "$conn" ] && exit_error "Connection name is missing"
    [ -z "$1"    ] && exit_error "Specification is missing"

    if ! tail -1 $ipsec_conf | egrep -q "^[ ]*$"; then
        echo "" >> $ipsec_conf
    fi

    echo "conn $conn" >> $ipsec_conf
    echo $@ | tr -d ' ' | tr '~' ';' | sed 's/\([^|]*\)|/\t\1\n/g' \
        >> $ipsec_conf

    # For debugging purposes.
    echo "The following connection was added to $ipsec_conf:"
    sed -n "/conn $conn\$/,/^\$/p" $ipsec_conf
}

# ipsec_del_connection
#
# INPUT
# Connection name
#
# OUTPUT
# None
#
# DESCRIPTION
# This function removes connection of given name from IPsec
# configuration file ($ipsec_conf).
#
function ipsec_del_connection {

    local conn=$1

    [ -z "$conn" ] && exit_error "Connection name is missing"

    sed -i "/conn $conn\$/,/^\$/d" $ipsec_conf
}

# ipsec_add - Attempt to negotiate a new IPsec SA using ipsec
#
# INPUT
# Destination port number
#
# OUTPUT
# None
#
# DESCRIPTION
# This function attempts to negotiate a IPsec SA with a remote node using the
# "ipsec" daemon.  The function does this by using a lblnet_tst_server on NS
# to remotely control listening on destination TCP port number. Once NS is
# waiting for new connections the function tries to connect to NS from TOE.
# which triggers a SPD rule in the IPsec subsystem which sends a SA "acquire"
# message to the "ipsec" daemon which then attempts to negotiate an IPsec SA
# with NS. If the "ipsec" deamon is unable to negotiate a SA with the remote
# host the connection will fail.
#
function ipsec_add_sa {
    local port=$1

    [ -z "$port" ] && exit_error "Destination port is missing"

    # Tell NS to listen on desired port.
    str="detach;recv:ipv${ipsec_ip_ver},tcp,${port},0;"
    if [ "$PPROFILE" == "lspp" ]; then
        runcon -t lspp_test_netlabel_t -l SystemLow -- \
            $ipsec_nc $ipsec_dst $ipsec_dst_port <<< "$str"
    else
        $ipsec_nc $ipsec_dst $ipsec_dst_port <<< "$str"
    fi

    # Create SA by connecting to secured port on NS.
    local result=1
    for attempt in 1 2; do
        if [ "$PPROFILE" == "lspp" ]; then
            runcon -t lspp_test_ipsec_t -l SystemLow -- \
                $ipsec_nc $ipsec_dst $port <<< "Hello"
        else
            $ipsec_nc $ipsec_dst $port <<< "Hello"
        fi

        result=$?
        [ $result -eq 0 ] && break;
    done

    [[ $result != 0 ]] && exit_fail "Unable to establish a SA"
}

# ipsec_add_sa_verify - Verify that ipsec establishe SA correctly
#
# INPUT
# * Port number.
# * Protocol (ah or esp).
# * IKE version,
# * Phase1 algorithm,
# * Phase2 algorithm (only for IKEv1).
#
# OUTPUT
# none
#
# DESCRIPTION
# This function queries the kernel's SAD to see if the ipsec_add() function
# was successful in establishing a SA.  In addition this function checks to see
# if an audit record was generated when the SA was established.  If either the
# SA or the audit record is missing this function fails and calls the
# exit_fail() function.
#
function ipsec_add_sa_verify {

    local port=$1
    local protocol=$2
    local ikev=$3
    local p1_alg=$4
    local p2_alg=$5

    [ -z "$port"     ] && exit_error "Destination port is missing"
    [ -z "$protocol" ] && exit_error "Protocol is missing"
    [ -z "$ikev"     ] && exit_error "IKE version is missing"
    [ -z "$p1_alg"   ] && exit_error "Phase1 algorithm is missing"
    [ -z "$p2_alg"   ] && exit_error "Phase2 algorithm is missing"

    if [[ "$p1_alg" =~ aes ]]; then
        p1_cipher="aes"
        p1_mode=$(echo "$p1_alg" | sed 's/aes_\(ctr\|cbc\|gcm\).*/\1/g')
        p1_bits=$(echo "$p1_alg" | sed 's/aes_\(ctr\|cbc\|gcm_.\)\(128\|256\|192\).*/\2/g')
        p1_dh=$(echo "$p1_alg" | sed 's/aes_\(ctr\|cbc\|gcm_.\)\(128\|256\|192\)-sha1;\(.\+\)/\3/g')
    elif [[ "$p1_alg" =~ 3des ]]; then
        p1_cipher="3des"
        p1_mode="cbc"
        p1_bits="192"
        p1_dh=$(echo "$p1_alg" | sed 's/3des-sha1;\(.\+\)/\1/g')
    fi

    if [[ "$p2_alg" =~ aes ]]; then
        p2_cipher="aes"
        p2_mode=$(echo "$p2_alg" | sed 's/aes_\(ctr\|cbc\|gcm\).*/\1/g')
        p2_bits=$(echo "$p2_alg" | sed 's/aes_\(ctr\|cbc\|gcm_.\)\(128\|256\|192\).*/\2/g')
        p2_dh=$(echo "$p2_alg" | sed 's/aes_\(ctr\|cbc\|gcm_.\)\(128\|256\|192\)-sha1;\(.\+\)/\3/g')
    elif [[ "$p1_alg" =~ 3des ]]; then
        p2_cipher="des"
        p2_mode="cbc"
        p2_dh=$(echo "$p1_alg" | sed 's/3des-sha1;\(.\+\)/\1/g')
    fi

    # For debugging purposes.
    ip xfrm state

    # Check that there are some tunnels.
    [ -z "$(ip xfrm state)" ] && exit_fail "No SA found"

    # IKE version check (for IKEv2 only).
    if [ "$ikev1" == "2" ]; then
        grep "STATE_PARENT_I2" $pluto_log | grep -i "ikev2" || \
            exit_fail "Incorrect IKE version detected"
    fi

    # Check port.
    ip xfrm state | grep "$src $ipsec_dst dst $ipsec_src" -A 5 | \
        grep "tcp sport $port" || \
        exit_fail "Incorrect port detected"
    ip xfrm state | grep "$src $ipsec_src dst $ipsec_dst" -A 5 | \
        grep "tcp dport $port" || \
        exit_fail "Incorrect port detected"

    # Protocol check.
    egrep "(PARENT|IPsec) SA established" $pluto_log | grep -i $protocol || \
        exit_fail "Incorrect protocol detected"

    ip xfrm state | grep "src $ipsec_src dst $ipsec_dst" -A 5 | \
        sed -n '2p' | grep "proto $mode" || \
        exit_fail "Incorrect protocol detected"
    ip xfrm state | grep "src $ipsec_dst dst $ipsec_src" -A 5 | \
        sed -n '2p' | grep "proto $mode" || \
        exit_fail "Incorrect protocol detected"

    # Phase1 check.
    if [ "$ikev" == "1" ]; then
        egrep "(PARENT|ISAKMP) SA established" $pluto_log | grep -i $p1_cipher || \
            exit_fail "Incorrect IKE cipher detected"
        egrep "(PARENT|ISAKMP) SA established" $pluto_log | grep -i $p1_bits || \
            exit_fail "Incorrect IKE bit size detected"
    else
        egrep "STATE_PARENT_I2:" $pluto_log | grep -i $p1_cipher || \
            exit_fail "Incorrect IKE cipher detected"
        egrep "STATE_PARENT_I2:" $pluto_log | grep -i $p1_bits || \
            exit_fail "Incorrect IKE bit size detected"
    fi
    if [ "$ikev" == "1" ]; then
        grep "ISAKMP SA established" $pluto_log | grep -i $p1_dh || \
            exit_fail "Incorrect IKE DH group detected"
    else
        grep "STATE_PARENT_I2" $pluto_log | grep -i $p1_dh || \
            exit_fail "Incorrect IKE DH group detected"
    fi

    # Phase2 check.
    if [ "$mode" == "esp" ] && [ "$ikev" == "1" ]; then

        grep "initiating Quick Mode" $pluto_log | grep -i $p2_cipher || \
            exit_fail "Incorrect Phase2 cipher detected"
        grep "initiating Quick Mode" $pluto_log | grep -i $p2_dh || \
            exit_fail "Incorrect Phase2 DH group detected"
        [[ "$p2_alg" =~ aes ]] && { \
            grep "initiating Quick Mode" $pluto_log | grep $p2_bits || \
            exit_fail "Incorrect Phase2 bit size detected"; }

        ip xfrm state | grep "src $ipsec_dst dst $ipsec_src" -A 5 | \
            sed -n '5p' | grep "enc ${p2_mode}(.?$p2_algo" || \
            exit_fail "Incorrect algorithm or mode detected"
        ip xfrm state | grep "src $ipsec_src dst $ipsec_dst" -A 5 | \
            sed -n '5p' | grep "enc ${p2_mode}(.?$p2_algo" || \
            exit_fail "Incorrect algorithm or mode detected"
    fi

    # Check audit event.
    ausearch -ts $audit_mark -m MAC_IPSEC_EVENT
    ausearch -ts $audit_mark -m CRYPTO_IPSEC_SA
    ausearch -ts $audit_mark -m CRYPTO_IKE_SA

    if [ "$ikev" == "1" ] && [ "$PPROFILE" == "lspp" ]; then
        ikev1_filter="sec_alg=1 sec_doi=1 sec_obj=staff_u:lspp_test_r:lspp_test_ipsec_t:s0"
    fi

    ausearch -ts $audit_mark -m MAC_IPSEC_EVENT | \
        egrep "op=SAD-add.*$ikev1_filter.*src=$normalized_ipsec_src.*dst=$normalized_ipsec_dst" || \
        exit_fail "Missing audit record"

    if [ "$mode" == "esp" ] && [ "$ikev" == "1" ]; then
        ausearch -ts $audit_mark -m CRYPTO_IPSEC_SA | \
            egrep -i "cipher=[3]?$p2_cipher.*ksize=$p2_size" || \
            exit_fail "Missing audit record"
    fi

    ausearch -ts $audit_mark -m CRYPTO_IKE_SA | \
        egrep -i "cipher=$p1_cipher.*ksize=$p1_size.*pfs=$p1_dh" || \
        exit_fail "Missing audit record"
}

# ipsec_remove_sa - Remove all IPsec SAs on the system
#
# INPUT
# none
#
# OUTPUT
# none
#
# DESCRIPTION
# This function attempts to flush/remove all SAs from the kernel IPsec
# subsystem, including the SAs negotiated in the ipsec_add_sa function.
# If this function can not flush/remove all the SAs from the kernel it
# will call the exit_fail function to signify failure.
#
function ipsec_remove_sa {
    ip xfrm state flush
    [[ $? != 0 ]] && exit_fail "Unable to remove the SA"
}

# ipsec_remove_sa_verify - Verify that the SAs have been removed
#
# INPUT
# none
#
# OUTPUT
# none
#
# DESCRIPTION
# This function queries the kernel's SAD to make sure the SAs have been removed
# and that an audit record was generated for the SAs established by the
# ipsec_add() function.  If either any SAs are found or the SA removal audit
# records are missing the function fails and the exit_fail() function is
# called.
#
function ipsec_remove_sa_verify {

    # Check that SA were removed.
    ip xfrm state | grep "$src $ipsec_dst dst $ipsec_src" && \
        exit_fail "Failed to remove SA"
    ip xfrm state | grep "$src $ipsec_src dst $ipsec_dst" && \
        exit_fail "Failed to remove SA"

    # Check audit event.
    ausearch -ts $audit_mark -m MAC_IPSEC_EVENT | \
        egrep "op=SAD-delete.*src=$normalized_ipsec_src.*dst=$normalized_ipsec_dst" || \
        exit_fail "Missing audit record"
}

# ipsec_destroy - flush both TOE and NS SAD and kill lblnet-tst-server
#
# INPUT
# none
#
# OUTPUT
# none
#
# DESCRIPTION
# This function flush ipsec SAD on TOE and NS and kills the remote lblnet_tst
# server (if needed). This is needed to reset the state at the beginning and
# the end of the tests.
#
function ipsec_destroy {
    # Flush TOE SAD.
    ip xfrm state flush

    # Flush NS SAD.
    if [ "$ipsec_dst" ]; then
        $ipsec_nc -w 3 "$ipsec_dst" "$ipsec_dst_port" <<< "ipsec:flush;"
        tstsvr_cleanup $ipsec_dst
    fi
}
