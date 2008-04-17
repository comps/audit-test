#!/bin/sh
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2006
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of version 2 the GNU General Public License as
#   published by the Free Software Foundation.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
###############################################################################


CheckVars ()
{
	if [ "$TESTPATH" ]
	then
		export TESTPATH
	else
		echo "You must enter a valid path to the testsuite."
		exit 255
	fi

	# Add "-v " to VERSION variable.  This is required for the
	# individual test functions.
	VERSION="-v $VERSION"
}


InitEnv ()
{
	VERSION=${VERSION:="1.2"}

	# Default independent tests
	INDTESTS="1 20 6 14 15 16 17 18 21 22 23 24 25 26 27 28 29"

	# Default dependent tests
	DEPTESTS="2 3 4 7 8 9 10 11 12 13 19 30 32 33 34 35 36"
	DEPTESTS="$DEPTESTS 37 38 43 44 45 46 47 48"

	if [ -z $TESTSUITE_OWNER_SECRET ]; then
		TESTSUITE_OWNER_SECRET="abc123"

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
	for K in `ls -1 testlib/test*`
	do
		. $K
	done
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
				exit
			fi
			;;
		-t )
			TESTORDER=`echo $2 | sed 's/,/ /g'`
			shift
			;;
		-u | -h )
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
	}
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
	"
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
CheckVars
RunTests


