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
# Verify that aide does not report any changes when there have not been
# any changes.  To perform this test first a database needs to be created
# then it is moved into place and a second run verifies that nothing new
# is found.

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

runcon -l SystemHigh -- aide -c ./aide-local.conf -i
mv aide.db.new.gz aide.db.gz

# test
runcon -l SystemHigh -- aide -c ./aide-local.conf -C

# verify
if [ $? -ne 0 ]; then
  cat aide.log
  exit_fail "aide did not complete its check run correctly"
fi

exit_pass

