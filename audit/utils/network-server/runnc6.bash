#!/bin/bash

hostext="$(hostname | awk 'BEGIN { FS = "." } { print $1 }')"
source ../netfilter/profile.$hostext

exec /usr/bin/nc -6 -w 2 $CATCHER_IPV6 $CATCHER_PORT6
exit
