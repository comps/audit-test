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
# 
# PURPOSE:
# Verify that aide will successfully notice an updated file, and that it
# reports an audit event about the change.  To test this first a database
# is created and then moved into place.  A test file is created and then is
# updated by changing its mls level to Secret and aide is re-run.  The
# test is successful if aide notices the change and issues an audit of
# the difference

source testcase.bash || exit 2

# setup
prepend_cleanup rm -f aide-local.conf aide.db.new.gz aide.db.gz aide.log
cat > aide-local.conf << EOF
# Sample aide.conf for audit testing

# The location of the database to be read.
database=file:$(pwd)/aide.db.gz

# The location of the database to be written.
database_out=file:$(pwd)/aide.db.new.gz

gzip_dbout=yes
verbose=5
report_url=file:$(pwd)/aide.log

# Just do md5 and sha256 hashes
LSPP = R+sha256

/usr/local LSPP
!$(pwd)
EOF

modified=../aide-testfile
prepend_cleanup rm $modified
touch $modified

runcon -l SystemHigh -- aide -c ./aide-local.conf -i
mv aide.db.new.gz aide.db.gz

auid=$(cat /proc/self/loginuid)
storedcon=$(ls -lZ $modified | awk '{print $4}')
chcon -l Secret $modified

# test
runcon -l SystemHigh -- aide -c ./aide-local.conf -C

# verify
if [ $? -eq 0 ]; then
  exit_fail "aide did not notice a changed file context properly"
fi

grep -q $modified aide.log || exit_fail "Modified file not found in aide.log"

msg1="added=0 removed=0 changed=1: exe=./usr/sbin/aide.*res=failed."
augrok -q type=ANOM_RBAC_INTEGRITY_FAIL msg_1=~"${msg1}" || \
  exit_fail "unable to find audit record containing $msg1"

exit_pass

