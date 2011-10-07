#!/bin/sh


#
#  08/17/11 - Jim Czyzak   - This script is based upon runEALtests.sh script.
#                            It has been modified to use the command file
#                            cc_commands and only include arguments that might
#                            be useful for the common criteria testing against
#                            the ospp in conjunction with the audit-test suite.
#                            Also includes code to turn off screen in
#                            /etc/profile for the test run as screen interferes
#                            with the su tests (screen is turned on in the
#                            in some evaluated configurations for purposes of
#                            securing display devices)

cd `dirname $0`
export LTPROOT=${PWD}
export TMPBASE="/tmp"
export TMP="${TMPBASE}/runalltests-$$"
export PATH="${PATH}:${LTPROOT}/../testcases/bin"
cmdfile="${LTPROOT}/../runtest/cc_commands"
pretty_prt=" "
alt_dir=0
quiet_mode=" "

usage()
{
	cat <<-END >&2
    usage: ./${0##*/} -c [-d tmpdir] [-f cmdfile ] [ -l logfile ]
                  -q [ -r ltproot ] [ -t duration ] [ -x instances ]

    -c              Run LTP under additional background CPU load.
    -d tmpdir       Directory where temporary files will be created.
    -f cmdfile      Execute user defined list of testcases.
    -h              Help. Prints all available options.
    -l logfile      Log results of test in a logfile.
    -p              Human readable format logfiles.
    -q              Print less verbose output to screen.
    -r ltproot      Fully qualified path where testsuite is installed.
    -t duration     Execute the testsuite for given duration in hours.
    -x instances    Run multiple instances of this testsuite.

    example: ./${0##*/} -i 1024 -m 128 -p -q  -l /tmp/resultlog.$$ -d ${PWD}
	END
exit
}

mkdir -p ${TMP}

cd ${TMP}
if [ $? -ne 0 ]; then
  echo "could not cd ${TMP} ... exiting"
  exit
fi

while getopts cd:f:h:l:pqr:t:x arg
do  case $arg in
    c)
            $LTPROOT/../testcases/bin/genload --cpu 1 2>&1 1>/dev/null &
            GenLoad=1 ;;

    d)      # append $$ to TMP, as it is recursively
            # removed at end of script.
            TMPBASE=$OPTARG;;
    f)        # Execute user defined set of testcases.
            cmdfile=$OPTARG;;

    h)      usage;;

    l)
            if [ ${OPTARG:0:1} != "/" ]
                        then
                                if [ -d $LTPROOT/results ]
                                then
                                        logfile="-l $LTPROOT/results/$OPTARG"
                                else
                                        mkdir -p $LTPROOT/results
                                        if [ $? -ne 0 ]
                                        then
                                                echo "ERROR: failed to create $LTPROOT/results"
                                                exit 1
                                        fi
                                        logfile="-l $LTPROOT/results/$OPTARG"
                                fi
                                alt_dir=1
            else
                                logfile="-l $OPTARG"
                        fi ;;

    p)      pretty_prt=" -p ";;

    q)      quiet_mode=" -q ";;

    r)      LTPROOT=$OPTARG;;

    t)      # In case you want to specify the time
            # to run from the command line
            # (2m = two minutes, 2h = two hours, etc)
            duration="-t $OPTARG" ;;

    x)      # number of ltp's to run
            instances="-x $OPTARG";;

    \?)     usage;;
    esac
done

if [ -z $PASSWD ]
then
        echo " "
        echo "ERROR:"
        echo "Please export enviroment variable PASSWD"
        echo "INFO: export PASSWD = 'root's password'"
    exit 1
fi

if [ -n "$instances" ]; then
  instances="$instances -O ${TMP}"
fi


# If user does not provide a command file select a default set of testcases
# to execute.
if [ -z $cmdfile ]
then
        cat ${LTPROOT}/../runtest/admin_tools > ${TMP}/alltests
else
    cat $cmdfile > ${TMP}/alltests
fi

# The fsx-linux tests use the SCRATCHDEV environment variable as a location
# that can be reformatted and run on.  Set SCRATCHDEV if you want to run
# these tests.  As a safeguard, this is disabled.
unset SCRATCHDEV
if [ -n "$SCRATCHDEV" ]; then
  cat ${LTPROOT}/../runtest/fsx >> ${TMP}/alltests
fi

# turn off screen in /etc/profile
tmpbkup=$(mktemp "/etc/profile.XXXXXX") || exit 1
cp -a /etc/profile $tmpbkup || exit 1
sed -i 's/\[ -w $(tty) \]/false/' /etc/profile

# display versions of installed software
${LTPROOT}/../ver_linux

${LTPROOT}/../bin/ltp-pan $quiet_mode -e -S $instances $duration -a $$ -n $$ $pretty_prt -f ${TMP}/alltests $logfile

if [ $? -eq 0 ]; then
  echo ltp-pan reported PASS
else
  echo ltp-pan reported FAIL
fi

if [ $alt_dir -eq 1 ]
then
        echo " "
        echo "###############################################################"
        echo " "
        echo " result log is in the $LTPROOT/results directory"
        echo " "
        echo "###############################################################"
        echo " "
fi
# restore /etc/profile
mv -f $tmpbkup /etc/profile

rm -rf ${TMP}
