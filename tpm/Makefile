###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2008
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

