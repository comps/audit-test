#!/bin/sh
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2008
###############################################################################


InitEnv ()
{
	VERSION=${VERSION:="1.2"}

	# Default independent tests
	INDTESTS="1 20 6 14 15 16 17 18 21 22 23 24 25 26 27 28 29"

	# Default dependent tests
	DEPTESTS="3 4 7 8 9 10 11 12 13 19 30 32 33 34 35 36"
	DEPTESTS="$DEPTESTS 37 38 43 44 45 46 47 48"

	if [ -z $TESTSUITE_OWNER_SECRET ];
	then
		TESTSUITE_OWNER_SECRET="abc123"

		# Getting the Public Endorsement Key (test2) is not allowed
		# when Ownership has already been taken.  Since it has not been
		# taken, run test2
		DEPTESTS="2 $DEPTESTS"

		# Clear the TPM (test5).  This is only done as part of
		# dependent testing and only when the user did not provide
		# an ownership password.
		DEPTESTS="$DEPTESTS 5"
	fi
	TESTSUITE_SRK_SECRET=${TESTSUITE_SRK_SECRET:="abc123"}
	> eout
	> sout

	export TESTSUITE_OWNER_SECRET TESTSUITE_SRK_SECRET

	TESTORDER=${TESTORDER:="$INDTESTS $DEPTESTS"}

	#
	# Source into this script, all the test scripts
	#
	for K in `find tests -type f -name "test??"`
	do
		. $K
	done

	if [ -z "$TESTPATH" ];
	then
		export TESTPATH=./tcg
	fi
		

	# Add "-v " to VERSION variable.  This is required for the
	# individual test functions.
	VERSION="-v $VERSION"
}


GetCmdOpts ()
{
	while [ "$#" -gt 0 ]
	do
		case "$1" in
		-ct)	# Do NOT clear the TPM
			CTFLAG=1
			;;
		-co)	# Clear Owner
			Test5
			exit 0
			;;
		-D )	# Run dependent tests only
			TESTORDER="$DEPTESTS"
			;;
		-id)
			IDFLAG=1
			RunTests
			exit 0
			;;
		-I )	# Run independent tests only
			TESTORDER="$INDTESTS"
			;;
		-m )
			# Display the manufacturer of the TPM hardware.
			Test6 07
			exit 0
			;;
		-o )
			# Determine if the TPM has installed an owner.
			Test6 12
			exit 0
			;;
		-p )
			if [ -d "$2/tpm" ]
			then
				TESTPATH=$2
				shift
			else
				echo "$2: Invalid path specified."
				echo -n "Could not located sub-directories"
				echo " [ tpm, transport, tspi, etc ]"
				exit 1
			fi
			;;
		-t )
			TESTORDER=`echo $2 | sed 's/,/ /g'`
			shift
			;;
		-u | -h | --help )
			Usage
			exit 0
			;;
		-v )
			VERSION="$2"
			shift
			;;
		esac
		shift
	done
}


RunTests ()
{
	SUCCESS_COUNT=0;
	FAIL_COUNT=0;
	RUN_COUNT=0;

	for K in $TESTORDER
	do
		Test$K
		if [ $? -eq 0 ]; then
			((SUCCESS_COUNT++))
		else
			((FAIL_COUNT++))
		fi
		((RUN_COUNT++))
	done

	[ ! "$IDFLAG" ] && {
		echo ""
		echo "Completed $RUN_COUNT tests, $SUCCESS_COUNT Successful, $FAIL_COUNT Failures."
		echo ""
		if [ $FAIL_COUNT -gt 0 ]; then
			return 1;
		fi
	}

	return 0;
}


Usage ()
{
	echo "$0 -p <path-to-tcg-dir> [-co][-ct][-D][-id][-I][-m][-o][-t <n,n,n>][-v <version>]
	-p: Path to tcg directory, usually /trousers/testsuite/tcg
	    Example: -p /trousers/testsuite/tcg
	-co: Clear the TPM, only.  No other tests performed.
	-ct: Do not clear the TPM after tests have run.
	-D: Run only tests that require TPM ownership and SRK
	-id: Do not run tests, but show which tests would be run
	-I: Run only tests that do not require TPM ownership or SRK
	-m: Show the manufacturer of the TPM
	-o: Show status of TPM ownership
	-t: Run specific tests instead of default tests. Supply test numbers
	    separated by commas.
	    Example: -t 4,8,9,30 would run test4, test8, test9 and test30
	-u: Usage.
	-v: Define the TSS version to test.  The default is 1.2
	    Example: -v 1.2

Environment variables include:
	CTFLAG TESTORDER TESTSUITE_OWNER_SECRET TESTSUITE_SRK_SECRET

See tpmts.man for more information."
}

CheckTPM ()
{
	# Check for tcsd
	ps ax | grep [t]csd > /dev/null
	if [ $? = 1 ]; then
		echo "The trousers service (tcsd) does not appear to be running"
		echo "Please start it (as root: \`service tcsd start\` )"
		exit 1
	fi

	# Check that tpm is accessible
	/usr/sbin/tpm_selftest > /dev/null
	if [ $? -ne 0 ]; then
		echo "Trousers is unable to communicate with the TPM."
		echo "Please be sure it is properly enabled in the BIOS."
		exit 1
	fi

	# Check the version that trousers thinks is available
	TPMVER=$(/usr/sbin/tpm_version | awk '/Chip/{print $3}' | awk -F . '{print $1"."$2}')
	if [ "-v $TPMVER" != "$VERSION" ]; then
		echo "There seems to be a version mismatch, this may affect the test results"
	fi

	# Check if the password is correct
	$TESTPATH/tpm/Tspi_TPM_OwnerGetSRKPubKey01 $VERSION > /dev/null 2> /dev/null
	if [ $? = 1 ]; then
		echo "The TPM owner password is not correct, check that"
		echo "TESTSUITE_OWNER_SECRET set correctly in your environment?"
		exit 1
	fi
}

#
# General Notes
#
# There seems to be some discrepency in terms.  For the purposes of this
# test, definitions are as follows:
#    Activated - The TPM has been turned on in BIOS.  This is different
#	then "enabled".  In some cases, (usually i386) the user has
#	the option of completely deactivating the TPM such that the OS
#	does not know it exists at all.  When activated, we mean that
#	the OS is aware of the TPM's existance (driver loaded,
#	dmesg() shows a TPM entry) and the TPM can minimally run a
#	self-test on itself.
#    Enabled - The TPM is turn on and ready for general use by the
#	software stack and user applications.
#
# There are a few errors that are fatal and do not warrant the test
# continuing because either the TPM is not activated (turned on in
# BIOS) or is not enabled (able to communicate with software).
# 	The first instance of error code 7 or 17 will cause this
#	script to exit.
#
# NOTE: change test execution order with "INDTEST", "DEPTEST" or
# "TESTORDER" variables.
#
# INDTEST - These tests do not require TPM ownership or any other
# dependencies to function successfully.
#
# DEPTEST - These tests require TPM ownership, SRK definitions, or
# have dependencies on other tests to function successfully.
#
# Use the "-id" option to get a list of tests in the order that
# they would normally be run.
#

InitEnv
GetCmdOpts "$@"
CheckTPM
RunTests
exit $?
