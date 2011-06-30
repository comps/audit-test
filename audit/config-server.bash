#!/bin/bash
# =============================================================================
#   Copyright 2010, 2011 International Business Machines Corp.
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
# =============================================================================

# This script querries the user for adresses (mac, ipv4, and ipv6), device
# names of the ethernet interfaces, network masks, and the superuser password
# for all 3 platforms needed to perform the netfilter tests. The script will
# acquire all the information needed for all 3 systems during it's run on the
# TOE (target of evaluation) platform. The only repeated question for the other
# two platforms is the superuser password.
# It will apply the addresses acquired to the correct interface on each
# platform and also create routes needed for the forwarding tests.
#
#
#
#
# This function sets the device interfaces on the TOE (target of evaluation)
# with the addresses obtained through the questioning in this script. Also
# the routes to the remote network server and the 3rd platform known as the
# catcher are added to tho route table
#
function setup_toe {
source /tmp/profile
ifconfig $LOCAL_DEV $LOCAL_IPV4 netmask $LNET4MASK
ifconfig $LOCAL_DEV inet6 add $LOCAL_IPV6/$LNET6MASK
ifconfig $LOCAL_DEV inet6 add $TOE_GLOBAL/$LNET6MASK
ifconfig $BRIDGE_FILTER $LOCAL_SEC_IPV4 netmask $SNET4MASK
ifconfig $BRIDGE_FILTER inet6 add $LOCAL_SEC_IPV6/$SNET6MASK
ifconfig $BRIDGE_FILTER inet6 add $TOE_SEC_GLOBAL/$SNET6MASK
route add -net $SECNET_IPV4 netmask $SNET4MASK dev $BRIDGE_FILTER
route -A inet6 add $LBLNET_SVR_IPV6 dev $LOCAL_DEV
route -A inet6 add $SECNET_SVR_IPV6 dev $BRIDGE_FILTER
route -A inet6 add $CATCHER_IPV6 dev $BRIDGE_FILTER
route -A inet6 add $PITCHER_IPV6 dev $LOCAL_DEV
}
#
# This function assigns the addresses obtained during the questioning
# of the script to the interfaces on the remote network server running
# the lblnet_tst_server application.
#

function setup_net_server {

source /tmp/profile
ifconfig $LBLNET_SVR_DEV $LBLNET_SVR_IPV4 netmask $LNET4MASK
ifconfig $LBLNET_SVR_DEV inet6 add $LBLNET_SVR_IPV6/$LNET6MASK
ifconfig $SECNET_SVR_DEV $SECNET_SVR_IPV4 netmask $SNET4MASK
ifconfig $SECNET_SVR_DEV inet6 add $SECNET_SVR_IPV6/$SNET6MASK
ifconfig $PITCHER_DEV inet6 add $PITCHER_IPV6/$LNET6MASK
route -A inet6 add $CATCHER_IPV6 gw $TOE_GLOBAL dev $PITCHER_DEV
}

#
# This function sets the 3rd platforms interface that connects to the
# secondary network with the addresses obtained earlier by this script.
# it then sets up a netcat listen on the ipv4 port specified and a netcat
# listen on the ipv6 port specified for the purpose of receiving packets
# during the ipv4 and ipv6 forwarding test. These packets are sent from
# a netcat script called by the lblnet_tst_server and forwarded through
# the TOE.
function setup_catcher {

source /tmp/profile
ifconfig $CATCHER_DEV $CATCHER_IPV4 netmask $SNET4MASK
ifconfig $CATCHER_DEV inet6 add $CATCHER_IPV6/$SNET6MASK
route add $LBLNET_SVR_IPV4 gw $LOCAL_SEC_IPV4 dev $CATCHER_DEV
route -A inet6 add $PITCHER_IPV6 gw $TOE_SEC_GLOBAL dev $CATCHER_DEV
nc -l $CATCHER_PORT4 &
nc -6 -l CATCHER_PORT6 &
}

#
# This function asks the tester for the addresss and device particulars needed
# to run the filtering tests. There are a significant number of addresses and
# device names needed to not only set up the networking configuration, but also
# to provide the information needed to input the chain rules in iptables,
# ip6tables, and ebtables.
#
function get_env_variables {

if test -f /usr/local/eal4_testing/audit-test/profile.sample
   then
   source /usr/local/eal4_testing/audit-test/profile.sample
fi

if test -f /tmp/profile
   then
   rm -f /tmp/profile
fi
touch /tmp/profile
RHOST="localhost"
echo "export RHOST=\"localhost\"" >> /tmp/profile
RHOST6="::1"
echo "export RHOST6=\"::1\"" >> /tmp/profile
MODE="$(ask "64 bit or 32 bit" "$MODE")"
echo "export MODE=$MODE" >> /tmp/profile
PPROFILE="$(ask "Which profile lspp(mls) or capp(base)" "$PPROFILE")"
echo "export PPROFILE=$PPROFILE" >> /tmp/profile
PATH="$PATH:."
echo "export PATH=\"\$PATH:.\"" >> /tmp/profile

PASSWD="$(ask "Superuser passwword")"
echo "export PASSWD=$PASSWD" >> /tmp/profile
echo ""

echo "The directory path to audit-test requested below is for the toe"
echo "the directory path to audit-test on the netserver should be the same"
echo "If the path on the netserver is different you will need to manually"
echo "edit the AUDITPATH environmental variable in the /tmp/profile file"
echo "on the netserver after the profile is copied to the netserver's /tmp"
echo "directory to reflect the correct path to the audit-tests directory"
echo ""

AUDITPATH="$(ask "Directory path of audit-test (include audit-test)" "$AUDITPATH")"
LOCAL_DEV="$(ask "Primary network device name of TOE" "$LOCAL_DEV")"
LOCAL_SEC_DEV="$(ask "Secondary network device name of TOE" "$LOCAL_SEC_DEV")"
LOCAL_SEC_MAC="$(ask "Secondary device mac address of TOE (mac/mask)" "$LOCAL_SEC_MAC")"
LOCAL_IPV4="$(ask "IPV4 address of TOE primary device" "$LOCAL_IPV4")"
LOCAL_IPV6="$(ask "IPV6 address of TOE primary device" "$LOCAL_IPV6")"
LOCAL_SEC_IPV4="$(ask "IPV4 address of TOE secondary device" "$LOCAL_SEC_IPV4")"
LOCAL_SEC_IPV6="$(ask "IPV6 address of TOE secondary device" "$LOCAL_SEC_IPV6")"
TOE_GLOBAL="$(ask "Global IPV6 address of TOE primary device" "$TOE_GLOBAL")"
TOE_SEC_GLOBAL="$(ask "Global IPV6 address of TOE secondary device" "$TOE_SEC_GLOBAL")"

echo "export AUDITPATH=\"$AUDITPATH\"" >> /tmp/profile
echo "export LOCAL_DEV=\"$LOCAL_DEV\"" >> /tmp/profile
echo "export LOCAL_SEC_DEV=\"$LOCAL_SEC_DEV\"" >> /tmp/profile
echo "export LOCAL_SEC_MAC=\"$LOCAL_SEC_MAC\"" >> /tmp/profile
echo "export LOCAL_IPV4=\"$LOCAL_IPV4\"" >> /tmp/profile
echo "export LOCAL_IPV6=\"$LOCAL_IPV6\"" >> /tmp/profile
echo "export LOCAL_SEC_IPV4=\"$LOCAL_SEC_IPV4\"" >> /tmp/profile
echo "export LOCAL_SEC_IPV6=\"$LOCAL_SEC_IPV6\"" >> /tmp/profile
echo "export TOE_GLOBAL=\"$TOE_GLOBAL\"" >> /tmp/profile
echo "export TOE_SEC_GLOBAL=\"$TOE_SEC_GLOBAL\"" >> /tmp/profile

LBLNET_SVR_IPV4="$(ask "Network server's primary IPV4 address" "$LBLNET_SVR_IPV4")"
LBLNET_SVR_IPV6="$(ask "Network server's primary IPV6 address" "$LBLNET_SVR_IPV6")"
REMOTE_IPV6_RAW="$LBLNET_SVR_IPV6"
LBLNET_SVR_DEV="$(ask "Network server's primary device name" "$LBLNET_SVR_DEV")"
LNET4MASK="$(ask "Network server's primary IPV4 mask" "$SNET4MASK")"
LNET6MASK="$(ask "Network server's primary IPV6 mask" "$SNET6MASK")"
SECNET_SVR_IPV4="$(ask "Network server's secondary IPV4 address" "$SECNET_SVR_IPV4")"
SECNET_SVR_IPV6="$(ask "Network server's secondary IPV6 address" "$SECNET_SVR_IPV6")"
SECNET_SVR_DEV="$(ask "Network server's secondary device name" "$SECNET_SVR_DEV")"
SECNET_SVR_MAC="$(ask "Network server's secondary mac address (mac/mask)" "$SECNET_SVR_MAC")"
SECNET_IPV4="$(ask "Network server's secondary IPV4 network address" "$SECNET_IPV4")"
SNET4MASK="$(ask "Network server's secondary IPV4 mask" "$SNET4MASK")"
SNET6MASK="$(ask "Network server's secondary IPV6 mask" "$SNET6MASK")"

echo "export LBLNET_SVR_IPV4=\"$LBLNET_SVR_IPV4\"" >> /tmp/profile
echo "export LBLNET_SVR_IPV6=\"$LBLNET_SVR_IPV6\"" >> /tmp/profile
echo "export REMOTE_IPV6_RAW=\"$LBLNET_SVR_IPV6\"" >> /tmp/profile
echo "export LBLNET_SVR_DEV=\"$LBLNET_SVR_DEV\"" >> /tmp/profile
echo "export LNET4MASK=\"$LNET4MASK\"" >> /tmp/profile
echo "export LNET6MASK=\"$LNET6MASK\"" >> /tmp/profile
echo "export SECNET_SVR_IPV4=\"$SECNET_SVR_IPV4\"" >> /tmp/profile
echo "export SECNET_SVR_IPV6=\"$SECNET_SVR_IPV6\"" >> /tmp/profile
echo "export SECNET_SVR_DEV=\"$SECNET_SVR_DEV\"" >> /tmp/profile
echo "export SECNET_SVR_MAC=\"$SECNET_SVR_MAC\"" >> /tmp/profile
echo "export SECNET_IPV4=\"$SECNET_IPV4\"" >> /tmp/profile
echo "export SNET4MASK=\"$SNET4MASK\"" >> /tmp/profile
echo "export SNET6MASK=\"$SNET6MASK\"" >> /tmp/profile

CATCHER_IPV4="$(ask "Catcher's secondary IPV4 address" "$CATCHER_IPV4")"
CATCHER_IPV6="$(ask "Catcher's secondary global IPV6 address" "$CATCHER_IPV6")"
CATCHER_DEV="$(ask "Catcher's secondary device name?" "$CATCHER_DEV")"

PITCHER_IPV6="$(ask "Network server's primary global IPV6 address" "$PITCHER_IPV6")"
PITCHER_DEV="$LBLNET_SVR_DEV"

BRIDGE_FILTER="$(ask "Name of bridge device created for the filter testing" "$BRIDGE_FILTER")"

echo "export CATCHER_IPV4=\"$CATCHER_IPV4\"" >> /tmp/profile
echo "export CATCHER_IPV6=\"$CATCHER_IPV6\"" >> /tmp/profile
echo "export CATCHER_DEV=\"$CATCHER_DEV\"" >> /tmp/profile
echo "export CATCHER_PORT4=\"4100\"" >> /tmp/profile
echo "export CATCHER_PORT6=\"4200\"" >> /tmp/profile
echo "export PITCHER_IPV6=\"$PITCHER_IPV6\"" >> /tmp/profile
echo "export PITCHER_DEV=\"$PITCHER_DEV\"" >> /tmp/profile
echo "export BRIDGE_FILTER=\"$BRIDGE_FILTER\"" >> /tmp/profile

}

echo_user () {
        echo >/dev/tty "$@"
}

ask () {
        echo_user
        echo_user -n "$1 [$2] ? "
        read res </dev/tty
        [ -z "$res" ] && res="$2"
        echo_user -n "$res (y/n)"
        read ret </dev/tty
        if [ "$ret" == "y" ]; then
            echo "$res"
        else
           ask "$1" "$2"
        fi
}

confirm () {
        res=$(ask "$1 (y/n)" "$2")
        case "$res" in
        [yYjJ]*) true ;;
        *) false ;;
        esac
}

die () {
        echo_user "FATAL: $*"
        exit 1
}

echo "Valid role names are: toe, netserver, catcher"
echo ""
echo "toe (target of evaluation) is the platform being certified"
echo ""
echo "netserver is the remote server where the lblnet_tst_server"
echo "    is being run"
echo ""
echo "catcher is the third platform that will be the recipient of packets"
exho "transmitted by the lblnet_tst_server during the forwarding tests."
echo ""
echo "This script has to be run on the toe first. It will obtain required"
echo "info for all three roles and create a file in /tmp named profile"
echo "The file profile on the toe should then be copied to the /tmp directory"
echo "of both the netserver and catcher prior to running this script on those"
echo " 2 platforms"
echo ""
SERVER_ROLE="$(ask "Which role does this server perform" "toe")"
if [[ "$SERVER_ROLE" == "toe" ]]; then
   if test -f /tmp/profile
      then
      source /tmp/profile
   else
     if test -f ./profile.sample
        then
        source ./profile.sample
     else
       echo "There is no sample profile to use for default answers or"
       echo "examples fo format. Either you are not running in the audit-test"
       echo "directory or the sample profile that is normally in the"
       echo "audit-test directory has been deleted"
       confirm "Do you want to continue anyway? " "n" || {
                    die "Configuration aborted."
                   }
     fi
   fi

      get_env_variables
      setup_toe
      echo "You should now check the /tmp/profile file for errors"
      echo "and if satisfied it is correct, copy it to both the"
      echo "netserver's, and catcher's /tmp directory"
      exit
fi
if [[ "$SERVER_ROLE" == "netserver" ]]; then
   if test -f /tmp/profile
      then
      PASSWD="$(ask "Superuser passwword")"
      echo "export PASSWD=$PASSWD" >> /tmp/profile
      setup_net_server
      exit
   else
      echo "/tmp/profile does not exist"
      echo "Copy profie from toe platform to /tmp"
      exit
   fi
fi
if [[ "$SERVER_ROLE" == "catcher" ]]; then
   if test -f /tmp/profile
      then
      PASSWD="$(ask "Superuser passwword")"
      echo "export PASSWD=$PASSWD" >> /tmp/profile
      source /tmp/profile
      setup_catcher
      exit
   else
      echo "/tmp/profile does not exist"
      echo "Copy profie from toe platform to /tmp"
      exit
   fi
fi
echo "Invalid role, role names are case sensitive"
exit
