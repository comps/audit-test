#!/bin/bash

TESTSUITE_OWNER_SECRET=abc123
TESTSUITE_SRK_SECRET=abc123
export TESTSUITE_OWNER_SECRET TESTSUITE_SRK_SECRET

./tcg/init/Tspi_TPM_TakeOwnership01 -v 1.2

./tsstests.sh -v 1.2
