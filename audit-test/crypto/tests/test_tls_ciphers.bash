#!/bin/bash -x
###############################################################################
#   Copyright (c) 2011 Red Hat, Inc. All rights reserved.
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
# SFRs: Cryptographic operation (FCS_COP.1(NET))
#
# AUTHOR: Ondrej Moris <omoris@redhat.com>
#
# DESCRIPTION: The TSF shall perform encryption, decryption, integrity
#              verification, peer authentication in accordance with the
#              following cryptographic algorithms, cryptographic key sizes
#              and applicable standards.
#
#              TLS using by the following TLS cipher strings as defined by
#              RFC5246.
#

source testcase.bash || exit 2
source tp_nss_functions.bash || exit 2

# TLS server and client process IDs.
server_pid=""
client_pid=""

# Port TLS server listens on.
tls_port=12345

# Log files.
server_log="server.log"
client_log="client.log"

# Translates cipher name into NSS format, ie. :WXYZ where 0xWX 0xYZ is
# hexadecimal identifier of the cipher.
#
# $1 - cipher
#
# Return 0 and print the "code" of the cipher.
#
function cipher_code {
    openssl ciphers -V $1 | awk '{print $1}' | sed 's/0x\(..\),0x\(..\)/:\1\2/'
}

# Print string of cipher codes in a corresponding group of ciphers
# except the code of a cipher given as a parameter.
#
# $1 - cipher
#
# Return 0 and print string of other cipher codes of a group.
#
function other_codes {
    local codes=""
    for cipher in ${ciphers[${scenario}]}; do
        [ "$1" != "$cipher" ] && codes="$codes$(cipher_code $cipher)"
    done
    echo $codes
}

# Start TLS server (openssl s_server) listening on fixed port (see
# $tls_port variable) and using given cihper. Server requests client
# certificates to accept its connection. Server process ID is stored
# in $server_pid and its log is store in $server_log.
#
# $1 - server nickname (for NSS)
# $2 - CA nickname (for NSS)
# $2 - ciphers allowed
#
# Return 0.
#
# Exit with error if parameters are missing or if the server cannot be
# started.
#
function tls_server_start {

    local name=$1
    local ca_name=$2
    local cipher=$3

    [ -z "$name"       ] && exit_error "TLS server name is missing!"
    [ -z "$ca_name"    ] && exit_error "CA name is missing!"
    [ -z "$cipher"     ] && exit_error "TLS ciphers are missing!"
    [ -z "$server_pid" ] || exit_error "TLS server is already running!"
    [ -z "$tls_port"   ] && exit_error "TLS port is missing!"


    coproc openssl s_server  \
        -Verify 1            \
        -accept $tls_port    \
        -cert $name.crt      \
        -key $name.key       \
        -CAfile $ca_name.crt \
        -state               \
        -cipher $cipher      > $server_log 2>&1

    server_pid=$!

    wait_for_cmd "nc -w 1 $LOCAL_IPV4 $tls_port <<< ''"

    if [ "$(pidof openssl)X" != "${server_pid}X" ]; then
        echo "==== BEGIN (server log) ===="
        cat $server_log
        echo "==== END (server log) ===="

        exit_error "Cannot start TLS server!"
    fi

    append_cleanup "kill -9 $server_pid"

    return 0
}

# Connect to TLS server (nss tstclnt) on a fixed port ($tls_port)
# using given cipher and client certificate and key. Connection
# keeps alive in background ($client_pid) if it succeeded and it
# waits for the server to terminate.
#
# $1 - client name (for certificate and key selection)
# $2 - cipher
#
# Return 0.
#
# Exit with error if parameters are missing.
#
function tls_client_connect {

    local name=$1
    local cipher=$2

    rm -f client.log

    [ -z "$name"     ] && exit_error "TLS server name is missing!"
    [ -z "$tls_port" ] && exit_error "TLS port is missing!"
    [ -z "$cipher"   ] && exit_error "TLS ciphers are missing!"

    /usr/lib64/nss/unsupported-tools/tstclnt -v -x -o \
        -h $LOCAL_IPV4   \
        -p $tls_port     \
        -d $nss_db       \
        -n $name         \
        -c $cipher       \
        -V tls1.0:tls1.2 \
        -W $nss_secret   <<<"" >$client_log 2>&1 &

    client_pid=$!

    wait_for_cmd "egrep '(Read from server 0 bytes|SSL_ERROR_NO_CYPHER_OVERLAP|PR_Poll returned 0x00)' $client_log"

    return 0
}

# Stop running TSL server (openssl s_server) by sending SIGTERM signal
# to server process ($server_pid). Wait for client to terminate its
# connection. Then outputs of both server and client are printed out
# and their content checked.
#
# Return 0 if the client connection was successfull, 1 if ciphers of
# the client and the server do not overlap
#
# Exit with error if parameters are missing or an unexpected results
# were found.
#
function tls_server_stop {

    local server_result=2
    local client_result=2

    [ -z "$server_pid" ] && exit_error "TLS server is not running!"
    [ -z "$server_log" ] && exit_error "TLS server log is missing!"

    kill $server_pid >/dev/null 2>&1
    wait $server_pid
    wait $client_pid

    local client_rc=$?

    server_pid=""
    client_pid=""

    echo "==== BEGIN (server log) ===="
    cat $server_log
    echo "==== END (server log) ===="

    echo "==== BEGIN (client log) ===="
    cat $client_log
    echo "==== END (client log) ===="

    if  [ $client_rc -eq 1 ]; then
        exit_error "TLS client was not started!!"
    else
        if grep -q "tstclnt: ready..." $client_log; then

            if [ $client_rc -eq 0 ]; then
                client_result=0
            elif [ $client_rc -eq 254 ]; then
                grep -q "SSL_ERROR_NO_CYPHER_OVERLAP" $client_log && \
                    client_result=1
            fi
        fi
    fi

    grep "verify return:1"  $server_log && server_result=0
    grep "no shared cipher" $server_log && server_result=1

    [ $client_result -eq 2 ] && exit_error "Unexpected client result!"
    [ $server_result -eq 2 ] && exit_error "Unexpected server result!"

    [ $server_result != $client_result ] && \
        exit_error "Server and client results do not match!"

    return $client_result
}

# Do TLS test by starting TLS server and connecting by TLS client,
# First, only given cipher is allowed by both server and client,
# the connection must pass. Then server allows all except given
# cipher and client allows only the cipher, the connection must
# fail. Finally the server allows given cipher only and client all
# ciphers except the one given, the connection must fail.
#
# $1 - server name
# $2 - client name
# #3 - cipher
#
# Return 0 if connection succeeded, 1 otherwise.
#
# Exit with error if parameters are missing.
#
function tls_test {

    local result=0
    local server=$1
    local client=$2
    local cipher=$3

    [ -z "$server" ] && exit_error "Missing server name!"
    [ -z "$client" ] && exit_error "Missing client name!"
    [ -z "$cipher" ] && exit_error "Missing cipher code!"

    other=$(other_codes $cipher)

    # Should pass.
    tls_server_start   "server" "CA" "$cipher"
    tls_client_connect "client" "$(cipher_code $cipher)"
    tls_server_stop    || result=1

    # Should fail.
    tls_server_start   "server" "CA" "ALL:!$cipher"
    tls_client_connect "client" "$(cipher_code $cipher)"
    tls_server_stop    && result=1

    # Should fail.
    tls_server_start   "server" "CA" "$cipher"
    tls_client_connect "client" "$other"
    tls_server_stop    && result=1

    return $result
}

#############
# Main test #
#############

# Testing scenario description.
scenario=$1

# Aggregation of ciphers.
declare -A ciphers

# Key exchange RSA.
ciphers["key_exchange_rsa"]="         AES128-SHA                    \
                                      AES256-SHA                    \
                                      DES-CBC3-SHA                  \
                                      AES128-GCM-SHA256             \
                                      AES128-SHA256                 \
                                     -AES256-GCM-SHA384             \
                                      AES256-SHA256                 "

# Key agreement Diffie-Hellman (RSA).
ciphers["key_agreement_dh_rsa"]="     DHE-RSA-AES128-SHA            \
                                      DHE-RSA-AES256-SHA            \
                                      EDH-RSA-DES-CBC3-SHA          \
                                     -DHE-RSA-AES256-GCM-SHA384     \
                                      DHE-RSA-AES256-SHA256         \
                                      DHE-RSA-AES128-GCM-SHA256     \
                                      DHE-RSA-AES128-SHA256         "

# Key agreement Diffie-Hellman (DSA).
ciphers["key_agreement_dh_dsa"]="     DHE-DSS-AES128-SHA            \
                                      EDH-DSS-DES-CBC3-SHA          \
                                      DHE-DSS-AES256-SHA            \
                                     -DHE-DSS-AES256-GCM-SHA384     \
                                     -DHE-DSS-AES256-SHA256         \
                                     -DHE-DSS-AES128-GCM-SHA256     \
                                     -DHE-DSS-AES128-SHA256         "

# Key agreement EC Diffie-Hellman (RSA).
ciphers["key_agreement_ecdh_rsa"]="   ECDH-RSA-AES128-SHA           \
                                      ECDH-RSA-DES-CBC3-SHA         \
                                      ECDH-RSA-AES256-SHA           \
                                     -ECDH-RSA-AES128-GCM-SHA256    \
                                     -ECDH-RSA-AES128-SHA256        \
                                     -ECDH-RSA-AES256-GCM-SHA384    \
                                     -ECDH-RSA-AES256-SHA384        "

# Key agreement EC Diffie-Hellman (ECDSA).
ciphers["key_agreement_ecdh_ecdsa"]=" ECDH-ECDSA-AES128-SHA         \
                                      ECDH-ECDSA-AES256-SHA         \
                                      ECDH-ECDSA-DES-CBC3-SHA       \
                                     -ECDH-ECDSA-AES128-GCM-SHA256  \
                                     -ECDH-ECDSA-AES128-SHA256      \
                                     -ECDH-ECDSA-AES256-GCM-SHA384  \
                                     -ECDH-ECDSA-AES256-SHA384      "

# Key agreement EC Diffie-Hellman Ephemeral (RSA).
ciphers["key_agreement_ecdhe_rsa"]="  ECDHE-RSA-AES128-SHA          \
                                      ECDHE-RSA-AES256-SHA          \
                                      ECDHE-RSA-DES-CBC3-SHA        \
                                      ECDHE-RSA-AES128-GCM-SHA256   \
                                      ECDHE-RSA-AES128-SHA256       \
                                      ECDHE-RSA-AES256-GCM-SHA384   \
                                     -ECDHE-RSA-AES256-SHA384       "

# Key agreement EC Diffie-Hellman Ephemeral (ECDSA).
ciphers["key_agreement_ecdhe_ecdsa"]="ECDHE-ECDSA-AES128-SHA        \
                                      ECDHE-ECDSA-AES256-SHA        \
                                      ECDHE-ECDSA-DES-CBC3-SHA      \
                                      ECDHE-ECDSA-AES128-GCM-SHA256 \
                                      ECDHE-ECDSA-AES128-SHA256     \
                                      ECDHE-ECDSA-AES256-GCM-SHA384 \
                                     -ECDHE-ECDSA-AES256-SHA384     "

# Mapping of key types to test scenarios.
case "$scenario" in
    "key_exchange_rsa")
        ca_key_type="rsa"
        key_type="rsa"
        ;;
    "key_agreement_dh_rsa")
        ca_key_type="rsa"
        key_type="rsa"
        ;;
    "key_agreement_dh_dsa")
        ca_key_type="dsa"
        key_type="dsa"
        ;;
    "key_agreement_ecdh_rsa")
        ca_key_type="rsa"
        key_type="ecdsa nistp256"
        ;;
    "key_agreement_ecdh_ecdsa")
        ca_key_type="ecdsa nistp256"
        key_type="ecdsa nistp256"
        ;;
    "key_agreement_ecdhe_rsa")
        ca_key_type="rsa"
        key_type="rsa"
        ;;
    "key_agreement_ecdhe_ecdsa")
        ca_key_type="ecdsa nistp256"
        key_type="ecdsa nistp256"
        ;;
    *)
        exit_error "Incorrect scenario!"
        ;;
esac

# Initialize NSS DB and relatives.
nss_init || exit_error "Cannot initialize NSS DB!"

# Schedule NSS DB removal.
append_cleanup "nss_destroy"

# Generate self-signed CA.
nss_generate_pair "CA" "cn=CA,dc=mydomain,dc=com" "$ca_key_type" || \
    exit_error "Cannot generate CA!"

# Generate server and client certificates and keys.
nss_generate_pair "client" "cn=$(hostname)" "$key_type" "CA" || \
    exit_error "Cannot generate keys for client!"

nss_generate_pair "server" "cn=$LOCAL_IPV4" "$key_type" "CA" || \
    exit_error "Cannot generate keys for server!"

# Server and client logs are used during the test.
append_cleanup "rm -f $server_log"
append_cleanup "rm -f $client_log"

# Execute cipher testing.
server="server"
client="client"

for cipher in ${ciphers[${scenario}]}; do
    [[ $cipher =~ ^-.*$ ]] && continue
    tls_test "server" "client" $cipher || \
        exit_fail "Connection failed ($cipher)"
done

# No problems encountered.
exit_pass
