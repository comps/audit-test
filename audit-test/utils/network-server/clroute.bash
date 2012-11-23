#!/bin/bash

hostext="$(hostname | awk 'BEGIN { FS = "." } { print $1 }')"
source ../netfilter/profile.$hostext

ifconfig $SECNET_SVR_DEV $SECNET_SVR_IPV4 netmask $SNET4MASK
ifconfig $SECNET_SVR_DEV up
route del -net $SECNET_IPV4 netmask $SNET4MASK gw $LOCAL_IPV4 dev $PITCHER_DEV
exit
