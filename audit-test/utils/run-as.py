#!/usr/bin/python
###############################################################################
#   Copyright (c) 2014 Red Hat, Inc. All rights reserved.
#
#   This copyrighted material is made available to anyone wishing
#   to use, modify, copy, or redistribute it subject to the terms
#   and conditions of the GNU General Public License version 2.
#
#   This program is distributed in the hope that it will be
#   useful, but WITHOUT ANY WARRANTY; without even the implied
#   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#   PURPOSE. See the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public
#   License along with this program; if not, write to the Free
#   Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
#   Boston, MA 02110-1301, USA.
###############################################################################
#
# AUTHOR: Jiri Jaburek <jjaburek@redhat.com>
#
# This tool is a simple wrapper doing setuid(), setgid() or setgroups(),
# executing specified command (with arguments) afterwards.
#
# 'su' is unable to set gid in some implementations, the -g option works as -G,
# 'runuser' (new coreutils) includes gid in suppl. groups,
# so the only clean solution is to use this custom wrapper
#
# This tool is also suitable for cases where 'su' simply cannot be used, like
# executing commands under users with locked accounts (ie. "bin", uid 1)
#
# Options:
#   -u <user>    or  --user <user>
#   -g <group>   or  --group <group>
#   -G <groups>  or  --supp-groups <groups>
# groups: <group1>[,group2]...
#
# All options take both names (getent) and numeric ids.
# The -G option can have empty argument (doing setgroups([]), essentially).
#

import os, sys
import getopt
import pwd, grp

# argument parsing

if len(sys.argv) < 2:
    print >> sys.stderr, "not enough arguments"
    sys.exit(2)

try:
    opts, args = getopt.getopt(sys.argv[1:], 'u:g:G:',
                               ["user=", "group=", "supp-groups="])
except getopt.GetoptError as err:
    print >> sys.stderr, str(err)
    sys.exit(2)

user = group = supp_groups = None
for opt, arg in opts:
    if opt in ("-u", "--user"):
        user = arg
    elif opt in ("-g", "--group"):
        group = arg
    elif opt in ("-G", "--supp-groups"):
        supp_groups = arg
    else:
        assert False

# name -> id translation

if user:
    if not user.isdigit():
        user = pwd.getpwnam(user).pw_uid
    else:
        user = int(user)

if group:
    if not group.isdigit():
        group = grp.getgrnam(group).gr_gid
    else:
        group = int(group)

if supp_groups:
    # may be empty to denote 'no supplementary groups'
    if supp_groups == '':
        supp_groups = []
    else:
        grps = []
        for g in supp_groups.split(','):
            if not g.isdigit():
                g = grp.getgrnam(g).gr_gid
            else:
                g = int(g)
            grps.append(g)
        supp_groups = grps

# the actual user/group/supp_groups switch
# note: needs to be done in reverse

if supp_groups != None:
    os.setgroups(supp_groups)
if group != None:
    os.setgid(group)
if user != None:
    os.setuid(user)

# user-provided command execution

os.execvp(args[0], args)

sys.exit(1)  # shouldn't occur
