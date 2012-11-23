#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2007
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

# File History
# 11/30/2010
# This file is identical to the one in the network directory by the same name
# however two new routines have been added to support the network filtering
# functional testing.  Those routines are do_ping, and do_nc.
#

source testcase.bash || exit 2

######################################################################
# global variables
######################################################################

# NOTE: these are not truly global since this file is sourced from inside
#       run_test(), so declare them with "declare"

# audit record fields
declare log_mark success
declare uid=0 euid=0 suid=0 fsuid=0
declare gid=0 egid=0 sgid=0 fsgid=0
declare result=0

######################################################################
# common functions
######################################################################

# usage: check_result <success case> <result> <exit value> <testcase number>
function check_result {
    declare suc=$1 res=$2 ext=$3 err_name=$4
    declare err

    if [[ -n $err_name ]]; then
        err=$(get_error_code $err_name)
    fi

    # yes/no set in common startup, so we can assume only two cases
    case $suc in
        success)
             [[ $res != 0 ]] && exit_error "unexpected test result"
	     ;;
        fail)
	     if [[ $res == 0 ]]; then
		 exit_fail "operation should have been denied"
	     elif [[ $res != 1 ]]; then
		 exit_error "unexpected test result"
	     fi
             [[ $ext != $err ]] && exit_error "unexpected test error"
             # audit represents errors as negative numbers so fixup the global
             # field value
             exitval=-$(get_error_code_raw $err_name)
	     ;;
    esac
}

# usage: get_error_code_raw <error_name, e.g. EPERM>
#  this is a private function and should not be called outside the scope of
#  this file
function get_error_code_raw {
    case $1 in
	ERESTARTSYS)
             # XXX - this is to workaround a kernel audit ?bug?
             echo "512"
	     ;;
	*)
             gcc -E -dM /usr/include/asm-generic/errno.h | grep $1 | awk '{print $3}'
	     ;;
    esac
}

# usage: get_error_code <error_name, e.g. EPERM>
function get_error_code {
    case $1 in
	ERESTARTSYS)
             # XXX - this is to workaround a kernel audit ?bug?
             get_error_code_raw EINTR
	     ;;
	*)
             get_error_code_raw $1
	     ;;
    esac
}

# usage: get_sockcall_num <syscall, e.g. connect>
function get_sockcall_num {
    gcc -E -dM /usr/include/linux/net.h | grep -i SYS_$1 | awk '{print $3}'
}

# usage: get_sockcall_num_hex <syscall, e.g. connect>
function get_sockcall_num_hex {
    printf "%x" $(get_sockcall_num $1)
}

# send a ping so we can test filtering on ICMP packets
function do_ping {
   declare ipv_arg=$2 host_arg=$3 intf=$4
   declare rc
   case $ipv_arg in
        ipv4)
            rc=$(ping -c 3 -w 3 "$1" | grep -cw 0%)
            ;;
        ipv6)
            if [[ $host_arg == local ]]; then
                 rc=$(ping6 -c 3 -w 3 "$1" | grep -cw 0%)
            else
                 rc=$(ping6 -I $intf -c 3 -w 3 "$1" | grep -cw 0%)
            fi
            ;;
           *)
            exit_error
            ;;
   esac

   pidnum=$(ps -C run.bash -o pid=)
   if [[ $rc != 0 ]]; then
     rc=0
     printf "%d %d %d\n", "$rc" "$rc" "$pidnum"
   else
     rc=1
     printf "%d %d %d\n", "$rc" "$rc" "$pidnum"
   fi
   return $rc
   }

# This function causes netcat data to be sent. Doing this over the local
# loopback device allows us to generate various TCP flags and states we
# need to test in the filtering

function do_nc {
   declare ipv_arg=$2 tnum=$3 port=$4
   declare rc
   declare data_str="This string simply provides data to send over netcat"
   case $ipv_arg in
        ipv4)
            if [[ $tnum == 47 ]]; then
               nc -l $port &
               rc="$(nc -w 1 "$1" "$port" <<< $data_str)"
            else
               rc="$(nc -w 1 "$1" "$port")"
            fi
            ;;
        ipv6)
            if [[ $tnum == 48 ]]; then
               nc -l $port &
               rc="$(nc -6 -w 1 "$1" "$port" <<< $data_str)"
            else
               rc="$(nc -6 -w 1 "$1" "$port")"
            fi
            ;;
           *)
            exit_error
            ;;
   esac

   pidnum=$(ps -C run.bash -o pid=)
   if [[ 46 < $tnum && $tnum < 49 ]]; then
      printf "%d %d %d\n", "$rc" "$rc" "$pidnum"
      return $rc
   fi
   if [[ $rc != 0 ]]; then
     rc=0
     printf "%d %d %d\n", "$rc" "$rc" "$pidnum"
   else
     rc=1
     printf "%d %d %d\n", "$rc" "$rc" "$pidnum"
   fi
   return $rc
}

######################################################################
# common startup
######################################################################
