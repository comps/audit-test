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
source testcase.bash || exit 2
source tp_ipsec_functions.bash || exit 2
source tp_nss_functions.bash || exit 2

# Remote testing script.
remote_script="ipsec.bash"

# Test port.
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

# Backup configuration file.
ipsec_backup_conf
prepend_cleanup "ipsec_restore_conf"

remote_call "$remote_script" "ipsec_backup_conf"
prepend_cleanup "remote_call $remote_script ipsec_restore_conf"

# Backup secret file.
ipsec_backup_secrets
prepend_cleanup "ipsec_restore_secrets"

remote_call "$remote_script" "ipsec_backup_secrets"
prepend_cleanup "remote_call $remote_script ipsec_restore_secrets"

# Test variables initialization.
ipsec_init  "$ipv"
prepend_cleanup "ipsec_destroy"

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

    ipsec_set_secrets "PSK" "'secret'"
    remote_call "$remote_script" "ipsec_set_secrets,PSK,'secret'"

    leftcert=""
    rightcert=""
    authby="secret"

elif [ "$peer_auth" == "RSA" ]; then

    ipsec_set_secrets "RSA" "toe"
    remote_call "$remote_script" "ipsec_set_secrets,RSA,ns"

    leftcert="leftcert=toe"
    rightcert="rightcert=ns"
    authby="rsasig"

    # Init NSS DB.
    nss_init "/etc/ipsec.d/" || exit_error
    prepend_cleanup "nss_destroy"

    remote_call "nss.bash" "nss_init,/etc/ipsec.d"
    prepend_cleanup "remote_call nss.bash nss_destroy,/etc/ipsec.d/"

    # Generate CA pair.
    nss_generate_pair "CA" "cn=CA" "rsa" || exit_error

    # Generate client and server pairs.
    nss_generate_pair "toe" "cn=toe" "rsa" "CA" || exit_error
    nss_generate_pair "ns"  "cn=ns"  "rsa" "CA" || exit_error

    # Distribute credentials.
    [ -z "$NS_PASSWD" ] && NS_PASSWD=$PASSWD
    cp toe.crt ns.p12 /tmp/
    chmod a+r /tmp/toe.crt /tmp/ns.p12
    remote_call "functions.bash" \
        "scp,eal@${LOCAL_IPV4}:/tmp/toe.crt,/tmp/,$NS_PASSWD"
    remote_call "functions.bash" \
        "scp,eal@${LOCAL_IPV4}:/tmp/ns.p12,/tmp/,$NS_PASSWD"

    # Import credentials.
    nss_import "crt" "ns.crt" "ns"
    nss_import "p12" "toe.p12"

    remote_call "nss.bash" "nss_import,crt,/tmp/toe.crt,toe,/etc/ipsec.d,/etc/ipsec.d/nsspassword"
    remote_call "nss.bash" "nss_import,p12,/tmp/ns.p12,/etc/ipsec.d,/etc/ipsec.d/nsspassword"
else
    exit_error "Unexpected peer authentication type!"
fi

# Local configuration.
stop_service "ipsec"
ipsec_add_connection "test"               \
    "auto=route                          |\
     authby=$authby                      |\
     type=transport                      |\
     left=$ipsec_src                     |\
     right=$ipsec_dst                    |\
     ikev2=$ikev2                        |\
     ike=${p1_enc}-${p1_auth}            |\
     phase2=$p2_type                     |\
     phase2alg=$phase2alg                |\
     labeled_ipsec=$labeled_ipsec        |\
     connaddrfamily=ipv$ipv              |\
     leftprotoport=tcp                   |\
     rightprotoport=tcp/$ipsec_test_port |\
     policy_label=$policy_label          |\
     $leftcert                           |\
     $rightcert                          |"

append_cleanup "restart_service ipsec"
prepend_cleanup "ipsec_del_connection test"

start_service "ipsec"

# Remote configuration.
remote_call "$remote_script" "ipsec_stop"

# We have to remove ; from connection specification because it is
# a control character for lbltst_server.
r_phase2alg=$(echo $phase2alg | sed 's/;/~/g')
r_p1_auth=$(echo $p1_auth | sed 's/;/~/g')

remote_call "$remote_script" "ipsec_add_connection,test,        \
     auto=route                          |\
     authby=$authby                      |\
     type=transport                      |\
     left=$ipsec_src                     |\
     right=$ipsec_dst                    |\
     ikev2=$ikev2                        |\
     ike=${p1_enc}-${r_p1_auth}          |\
     phase2=$p2_type                     |\
     phase2alg=$r_phase2alg              |\
     labeled_ipsec=$labeled_ipsec        |\
     connaddrfamily=ipv$ipv              |\
     leftprotoport=tcp                   |\
     rightprotoport=tcp/$ipsec_test_port |\
     policy_label=$policy_label          |\
     $leftcert                           |\
     $rightcert                          |"

append_cleanup "remote_call $remote_script ipsec_restart"
prepend_cleanup "remote_call $remote_script ipsec_del_connection,test"

remote_call "$remote_script" "ipsec_start"

# Wait for SPD to set-up.
sleep 5

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
