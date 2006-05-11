#!/usr/bin/awk -f
# =============================================================================
# (c) Copyright Hewlett-Packard Development Company, L.P., 2005
# Written by Aron Griffis <aron@hp.com>
#
#   This program is free software;  you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY;  without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
#   the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program;  if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
# =============================================================================
/^TALLIED RESULTS/ { exit }     # sanity check

/^ *[0-9]+ pass \([0-9]+%\)$/ { pass += $1 }
/^ *[0-9]+ fail \([0-9]+%\)$/ { fail += $1 }
/^ *[0-9]+ error \([0-9]+%\)$/ { error += $1 }

END {
    total = pass + fail + error
    if (total > 0) {
       print ""
       print "TALLIED RESULTS"
       print ""
       printf "%4d pass (%d%%)\n", pass, pass*100/total
       printf "%4d fail (%d%%)\n", fail, fail*100/total
       printf "%4d error (%d%%)\n", error, error*100/total
       print "------------------"
       printf "%4d total\n", total
       print ""
    } else {
       print ""
       print "There are no results to be displayed"
       print ""
    }
}
