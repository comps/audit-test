#!/bin/bash -x
###############################################################################
#   Copyright (c) 2014 Red Hat, Inc. All rights reserved.
#
#   This copyrighted material is made available to anyone wishing
#   to use, modify, copy, or redistribute it subject to the terms
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

source testcase.bash || exit 2
source tp_ipsec_functions.bash || exit 2
source tp_nss_functions.bash || exit 2

######################################################################
# global variables
######################################################################

unset ipsec_nc
unset ipsec_src
unset ipsec_dst
unset ipsec_log_mark

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
#
function ipsec_init {

    ipsec_ip_ver=$1
    if [ "$ipsec_ip_ver" == "6" ]; then
        ipsec_src=$(ipsec_addr_local 6)   || exit_error
        ipsec_dst=$(ipsec_addr_remote 6)  || exit_error
    elif [ "$ipsec_ip_ver" == "4" ]; then
        ipsec_src=$(ipsec_addr_local 4)   || exit_error
        ipsec_dst=$(ipsec_addr_remote 4)  || exit_error
    else
        exit_error "Unexpected IP version (4 or 6 expected)"
    fi

    normalized_ipsec_src=$(ipsec_normalize_ip $(echo $ipsec_src))
    normalized_ipsec_dst=$(ipsec_normalize_ip $(echo $ipsec_dst))

    ipsec_log_mark=$(stat -c %s $audit_log)
    audit_mark="$(date "+%T")"
    ipsec_nc="nc -${ipsec_ip_ver} -w 30 -v "
}

# ipsec_gen_connection
#
# INPUT
# * (Global variables)
#
# OUTPUT
# None
#
# DESCRIPTION
# This function generates a connection specification based on global
# or environment variables and prints it out to stdout.
#
function ipsec_gen_connection {
    cat <<EOF
conn ${ipsec_src}-to-${ipsec_dst}
	auto=route
	authby=$authby
	type=transport
	left=$ipsec_src
	right=$ipsec_dst
	ikev2=$ikev2
	ike=${p1_enc}-${p1_auth}
	phase2=$p2_type
	phase2alg=$phase2alg
	labeled_ipsec=$labeled_ipsec
	connaddrfamily=ipv$ipv
	leftprotoport=tcp/$ipsec_test_port
	rightprotoport=tcp
	policy_label=$policy_label
	$leftcert
	$rightcert

EOF
}

# ipsec_add_sa - Attempt to negotiate a new IPsec SA using ipsec
#
# INPUT
# Source (TOE-side) port number
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
    local srcport=$1 dstport=

    [ -z "$srcport" ] && exit_error "Source port is missing"

    dstport=$(send_ns -${ipsec_ip_ver} remote "timeout,10;recvx,tcp")

    # Create SA by connecting to secured port on NS.
    local result=1
    for attempt in 1 2; do
        if [ "$PPROFILE" == "lspp" ]; then
            runcon -t lspp_test_ipsec_t -l SystemLow -- \
                $ipsec_nc $ipsec_dst -p $srcport $dstport <<< "Hello"
        else
            $ipsec_nc $ipsec_dst -p $srcport $dstport <<< "Hello"
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

    local srcport=$1
    local protocol=$2
    local ikev=$3
    local p1_alg=$4
    local p2_alg=$5

    [ -z "$srcport"  ] && exit_error "Source port is missing"
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
    ip -o xfrm state | grep "src $ipsec_dst dst $ipsec_src" | \
        grep "tcp dport $srcport" || \
        exit_fail "Incorrect port detected"
    ip -o xfrm state | grep "src $ipsec_src dst $ipsec_dst" | \
        grep "tcp sport $srcport" || \
        exit_fail "Incorrect port detected"

    # Protocol check.
    egrep "(PARENT|IPsec) SA established" $pluto_log | grep -i $protocol || \
        exit_fail "Incorrect protocol detected"

    ip -o xfrm state | grep "src $ipsec_src dst $ipsec_dst" | \
        grep "proto $mode" || \
        exit_fail "Incorrect protocol detected"
    ip -o xfrm state | grep "src $ipsec_dst dst $ipsec_src" | \
        grep "proto $mode" || \
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

        ip -o xfrm state | grep "src $ipsec_dst dst $ipsec_src" | \
            grep "enc ${p2_mode}(.?$p2_algo" || \
            exit_fail "Incorrect algorithm or mode detected"
        ip -o xfrm state | grep "src $ipsec_src dst $ipsec_dst" | \
            grep "enc ${p2_mode}(.?$p2_algo" || \
            exit_fail "Incorrect algorithm or mode detected"
    fi

    # Check audit event.
    wait_for_cmd "ausearch -ts $audit_mark -m MAC_IPSEC_EVENT"
    wait_for_cmd "ausearch -ts $audit_mark -m CRYPTO_IPSEC_SA"
    wait_for_cmd "ausearch -ts $audit_mark -m CRYPTO_IKE_SA"

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
    ip xfrm state flush || exit_fail "Unable to remove the SA"
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
    ip xfrm state | grep "src $ipsec_dst dst $ipsec_src" && \
        exit_fail "Failed to remove SA"
    ip xfrm state | grep "src $ipsec_src dst $ipsec_dst" && \
        exit_fail "Failed to remove SA"

    # Check audit event.
    wait_for_cmd "ausearch -ts $audit_mark -m MAC_IPSEC_EVENT"

    ausearch -ts $audit_mark -m MAC_IPSEC_EVENT | \
        egrep "op=SAD-delete.*src=$normalized_ipsec_src.*dst=$normalized_ipsec_dst" || \
        exit_fail "Missing audit record"
}


###############################################################################
#
# Tests for IPsec
#
# AUTHOR: Ondrej Moris <omoris@redhat.com>
#
# DESCRIPTION: This test verifies that various aspect of IPsec work
# as expected. The following functionality is tested:
#
#   * IKE version enforcement (v1 and v2),
#
#   * Security agreement (SA) type (esp or ah),
#
#   * IKE and "Phase2" algorithms ENC-AUTH for IKE and ESP Phase2 and
#     AUTH for AH Phase2 where
#     - ENC is 3des or aes_{cbc,ctr}{128,192,256},
#     - AUTH is sha1;modp{1024,...,8192} or sha1;dh{22,23,24},
#
#   * Peer authentication (PSK and RSA).
#
# Both IPv4 and IPv6 are tested. You can test any meaningful
# combination of aforementioned functionalities as follows:
#
# ipsec IPV IKE IKE-ENC IKE-AUTH SA-TYPE P2-ENC P2-AUTH PEER-AUTH
#
# IPV           := 4    | 6
# IKE           := 1    | 2
# SA-TYPE       := esp  | ah
# {IKE,P2}-ENC  := 3des | aes_{cbc,ctr}{128,192,256}
# {IKE,P2}-AUTH := none | sha1;modp{1024,...,8192} | sha1;dh{22,23,24}
# PEER-AUTH     := PSK  | RSA
#
# Related SFR(s): FCS_COP.1(NET), FCS_CKM.2(NET-IKE) and FTP_ITC.1.
#

# Test (src) port.
ipsec_test_port="4301"

# Test configuration (see test description).
ipv=$1
ikev=$2
p1_enc=$3
p1_auth=$4
p2_type=$5
p2_enc=$6
p2_auth=$7
peer_auth=$8

# Test variables initialization.
ipsec_init "$ipv"

# NS session (exclusive access).
s=$(send_ns -S remote) || exit_error
prepend_cleanup "send_ns -s \"$s\" -K remote"
# exclusively lock the NS, for up to 60 seconds
send_ns -s "$s" remote 'lock,ex;timeout,60' || exit_error

# IKE version enforcement (labeled ipsec is not available for IKEv2 yet).
labeled_ipsec="no"
policy_label="\"\""
if [ "$ikev" == "1" ]; then
    ikev2="no"

    if [ "$PPROFILE" == "lspp" ]; then
        labeled_ipsec="yes"
        policy_label="system_u:object_r:ipsec_spd_t:s0-s15:c0.c1023"
    fi
elif [ "$ikev" == "2" ]; then
    ikev2="yes"
else
    exit_error "Unexpected IKE version!"
fi

# SA type.
if [ "$p2_type" == "esp" ]; then
    phase2alg="${p2_enc}-${p2_auth}"
elif [ "$p2_type" == "ah" ]; then
    phase2alg="${p2_auth}"
else
    exit_error "Unexpected SA type!"
fi

# Peer Authentication.
if [ "$peer_auth" == "PSK" ]; then

    toe_secrets=": PSK 'secret'"
    ns_secrets=": PSK 'secret'"

    leftcert=""
    rightcert=""
    authby="secret"

elif [ "$peer_auth" == "RSA" ]; then

    toe_secrets=": RSA toe"
    ns_secrets=": RSA ns"

    leftcert="leftcert=toe"
    rightcert="rightcert=ns"
    authby="rsasig"

    # Init NSS DB.
    prepend_cleanup "nss_restore"
    nss_init "/etc/ipsec.d/" || exit_error

    prepend_cleanup "send_ns remote \"exec,crypto-nss,restore,/etc/ipsec.d\""
    send_ns remote "exec,crypto-nss,init,/etc/ipsec.d" || exit_error

    # Generate CA pair.
    nss_generate_pair "CA" "cn=CA" "rsa" || exit_error

    # Generate client and server pairs.
    nss_generate_pair "toe" "cn=toe" "rsa" "CA" || exit_error
    nss_generate_pair "ns"  "cn=ns"  "rsa" "CA" || exit_error

    # Import credentials.
    nss_import "crt" "ns.crt" "ns"
    nss_import "p12" "toe.p12"

    cat toe.crt | send_ns -i -s "$s" remote \
        "exec,crypto-nss,import,crt,-,toe,/etc/ipsec.d,/etc/ipsec.d/nsspassword" || exit_error
    cat ns.p12 | send_ns -i -s "$s" remote \
        "exec,crypto-nss,import,p12,-,/etc/ipsec.d,/etc/ipsec.d/nsspassword" || exit_error
else
    exit_error "Unexpected peer authentication type!"
fi

# Local configuration.
append_cleanup "ipsec_local_cleanup; restart_service ipsec"
{ ipsec_std_header; ipsec_gen_connection; } | ipsec_local_conf
echo "$toe_secrets" | ipsec_local_secrets
restart_service "ipsec"

# Remote configuration.
append_cleanup 'send_ns remote "exec,ipsec,cleanup"'
{ ipsec_std_header; ipsec_gen_connection; } | \
    send_ns -i -s "$s" remote 'exec,ipsec,conf' || exit_error
echo "$ns_secrets" | send_ns -i -s "$s" remote 'exec,ipsec,secrets' || exit_error
send_ns -s "$s" remote 'exec,ipsec,reload' || exit_error
check_ns -s "$s" remote -- '0 .*' || exit_error


# Attempt to negotiate SA using ipsec and verify the results.
ipsec_add_sa        "$ipsec_test_port"
ipsec_add_sa_verify "$ipsec_test_port"     \
                    "$p2_type"             \
                    "$ikev"                \
                    "${p1_enc}-${p1_auth}" \
                    "$phase2alg"

# Attempt to remove the SA and verify the results.
ipsec_remove_sa
ipsec_remove_sa_verify

# If we made it this far everything is okay.
exit_pass
