#!/bin/bash

VERSION=1.2
TESTSUITE_OWNER_SECRET=abc123
TESTSUITE_SRK_SECRET=abc123
export TESTSUITE_OWNER_SECRET TESTSUITE_SRK_SECRET

if [ \! -f tcg/common/common.o ]; then
	echo "Have you compiled the tests?"
	exit 1;
fi

if [ \! -x ./tcg/init/Tspi_TPM_TakeOwnership01 ]; then
	cd tcg/init;
	make Tspi_TPM_TakeOwnership01
	if [ $? -ne 0 ]; then
		echo "Unable to build Tspi_TPM_TakeOwnership01,"\
			"aborting test run"
		exit 1
	fi
fi

./tcg/init/Tspi_TPM_TakeOwnership01 -v $VERSION
if [ $? -ne 0 -a $? -ne 8 ]; then
	echo "Taking ownership failed, aborting test run"
	exit 1
fi

./tsstests.sh -v $VERSION -l out-`date +%m%d.%H%M`.log -e err-`date +%m%d.%H%M`.log $@
