#!/bin/bash

hostext="$(hostname | awk 'BEGIN { FS = "." } { print $1 }')"
source ../netfilter/profile.$hostext
declare roret

ifconfig $SECNET_SVR_DEV down
roret=$(route | grep $SECNET_IPV4)
if [[ -z $roret ]]; then
   route add -net $SECNET_IPV4 netmask $SNET4MASK gw $LOCAL_IPV4 dev $PITCHER_DEV
fi
/usr/bin/nc -v -w 2 $CATCHER_IPV4 $CATCHER_PORT4
sleep 2
echo "restoring normal route"
./clroute.bash
exit
