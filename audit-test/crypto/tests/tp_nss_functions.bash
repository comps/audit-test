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
# AUTHOR: Ondrej Moris <omoris@redhat.com>
#
# DESCRIPTION: NSS PKI helpers.

# NSS DB directory.
nss_db=""

# NSS DB secret file.
nss_secret=""

# Seed file for randomization.
nss_seed=""

# Initial certifiate serial number.
nss_serial=1001

# Initialize empty NSS DB in given directory and protected by password
# 'secret'. Moreover seed file for key generation is created.
#
# $1 - NSS DB directory (optional, by default it is temporary directory)
#
# Return 0 if NSS DB was initialized, 1 otherwise.
#
function nss_init {

    eval nss_db=$1
    [ -z "$1" ] && nss_db=$(mktemp -d)

    if [ -d $nss_db ]; then
        cp -r $nss_db "/tmp/$(basename $nss_db).backup"
        rm -rf $nss_db/*
    fi

    eval nss_secret=$nss_db/nsspassword
    eval nss_seed=$nss_db/seed.txt

    echo 'NSS Certificate DB:secret' > $nss_secret

    dd if=/dev/urandom of=$nss_seed bs=1M count=1

    # Create NSS DB.
    certutil -N -d $nss_db -f $nss_secret || return 1

    return 0
}

function nss_destroy {

    [ -z "$1" ] || nss_db="$1"

    [ -d "$nss_db" ] || { echo "NSS DB is not initialized!"; return 1; }

    backup_nss_db="/tmp/$(basename $nss_db).backup"
    rm -rf $nss_db
    if [ -d $backup_nss_db ]; then
        mv /tmp/$(basename $nss_db).backup $nss_db
        restorecon -vFR $nss_db
    fi

    rm -f *.p12 *.crt *.key

    return 0
}


# Parse given key string into parameters for certutil. Valid string are
# as follows: rsa, dsa, ecdsa nistp256, ecdsa nistp384 and ecdsa nistp521.
#
# $1 - key string
#
# Return 0 and print parameters to stdout.
#
function __parse_key {

    [ -z "$1" ] && { echo "Missing key paramater!"; return 1; }

    local alg=$(echo $1 | awk '{print $1}')
    local curve=$(echo $1 | awk '{print $2}')

    case "$alg" in
        "rsa")
            echo "-k rsa"
            ;;
        "dsa")
            echo "-k dsa"
            ;;
        "ecdsa")
            [[ "$curve" =~ nistp(256|384|521) ]] || \
                { echo "Incorrect elliptic curve!"; return 1; }
            echo "-k ec -q $curve"
            ;;
        *)  echo "Incorrect algorithm curve!"
            return 1
            ;;
    esac
}

# Generate a pair of a certificate and key signed by given CA in NSS DB,
# both certificate and key are exported in PEM format. If CA is not
# given, a pair for self-signed CA will be generated and its certificate
# in PEM format exported.
#
# $1 - name (is used also for certificate filename)
# $2 - subject name
# $3 - key string (see parse key function)
# $4 - CA to sign, if missing self-signed CA will ge generated
#
# Return 0 if a pair was generated (and exported) successfully, 1 if
# generation failed and 2 if its export failed.
#
# Exit with error is parameters are missing or NSS DB is not intialized.
#
function nss_generate_pair {

    [ -d "$nss_db"     ] || { echo "NSS DB is not initialized!"; return 1; }
    [ -f "$nss_secret" ] || { echo "NSS DB is not initialized!"; return 1; }
    [ -f "$nss_seed"   ] || { echo "NSS DB is not initialized!"; return 1; }

    local name=$1
    local subject=$2
    local type=$3
    local ca=$4

    [ -z "$name"    ] && { echo "Missing name!"; return 1; }
    [ -z "$subject" ] && { echo "Missing subject!"; return 1; }
    [ -z "$type"    ] && { echo "Missing type!"; return 1; }

    key=$(__parse_key "$type")

    if [ -z "$ca" ]; then
        expect -c "\
          spawn certutil -S -n $name -s $subject -2 -x -t CT,, -m 1000 \
            -v 120 -d $nss_db $key -f $nss_secret -z $nss_seed
          expect -re {.*Is this a CA certificate [y/N]?} {
            send \"y\r\"
            expect -re {.*enter to skip.*} {
              send \"\r\"
              expect -re {.*Is this a critical extension [y/N]?} {
                send \"N\r\"
                exp_wait
                exit 0
              }
            }
          }
          exit 1" || return 1

        certutil -d $nss_db -L -n "$name" -a > $name.crt
    else
        certutil -S -n $name -s $subject -c "$ca" -t "u,u,u" \
            -m $nss_serial -v 120 -d $nss_db $key -f $nss_secret -z $nss_seed || return 1
        pk12util -d $nss_db -o $name.p12 -n $name -w $nss_secret -k $nss_secret
        openssl pkcs12 -in $name.p12 -out $name.crt -nodes \
            -passin file:$nss_secret -clcerts
        openssl pkcs12 -in $name.p12 -out $name.key -nodes \
            -nocerts -passin file:$nss_secret

        nss_serial=$[$nss_serial+1]

        { [ -e $name.crt ] && [ -e $name.key ]; } || return 2
    fi

    return 0
}

function nss_import {

    local type="$1"
    local file="$2"
    local name="$3"

    [ "$type" == "crt" ] && shift

    [ -z "$3" ] || nss_db="$3"
    [ -z "$4" ] || nss_secret="$4"

    [ -d "$nss_db"     ] || { echo "NSS DB is not initialized!"; return 1; }
    [ -f "$nss_secret" ] || { echo "NSS DB is not initialized!"; return 1; }

    if   [ "$type" == "crt" ]; then
        certutil -A -n $name -t ',,' -d $nss_db -i $file || \
            { echo "Cannot import" ; return 1; }
    elif [ "$type" == "p12" ]; then
        pk12util -i $file -d $nss_db -k $nss_secret -w $nss_secret || \
            { echo "Cannot import" ; return 1; }
    else
        return 1
    fi

    return 0
}
