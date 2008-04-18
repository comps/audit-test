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
SUBDIRS=tcg
DISTFILES=tpmts.sh Makefile
OUTFILES=eout sout x1

all:	
	@set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i ; done

clean:
	@set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i clean ; done
	$(RM) $(OUTFILES)

tarball: clean
	tar -cpf ../tpmts-pkg.tar --exclude .svn \
		$(SUBDIRS) $(DEPDIR) $(INDDIR) $(DISTFILES)

