#!/bin/bash

VERSION=1.2
TESTSUITE_OWNER_SECRET=abc123
TESTSUITE_SRK_SECRET=abc123
export TESTSUITE_OWNER_SECRET TESTSUITE_SRK_SECRET

./tcg/init/Tspi_TPM_TakeOwnership01 -v $VERSION

if [ $? -ne 0 ]; then
	echo "Taking ownership failed, aborting test run"
	exit 1
fi

./tsstests.sh -v $VERSION -l out-`date +%m%d.%H%M`.log -e err-`date +%m%d.%H%M`.log $@
