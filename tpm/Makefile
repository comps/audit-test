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

DEPDIR=tests/DEP
INDDIR=tests/IND
TESTLIB=testlib

all:	library

library:
	mkdir -p ${TESTLIB}
	cp -p ${DEPDIR}/tpm/test* ${TESTLIB}
	cp -p ${DEPDIR}/tspi/test* ${TESTLIB}
	cp -p ${INDDIR}/tpm/test* ${TESTLIB}
	cp -p ${INDDIR}/tspi/test* ${TESTLIB}

tarball:
	tar -cpf ../tpmts-pkg.tar testlib tpmts.sh
