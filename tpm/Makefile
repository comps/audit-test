###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2008
###############################################################################

TPMTSVER:=$(shell awk '/^Version/{print $$2}' tpmts.spec)
DEPDIR=tests/DEP
INDDIR=tests/IND
SUBDIRS=tcg
DISTFILES=Makefile tpmts.sh tpmts.man tpmts.spec
TARFILE=tpmts-$(TPMTSVER).tar.gz
DESTDIR?=/usr/local/tpmts/

all:	
	@set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i ; done

clean:
	@set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i clean ; done

install: all
	mkdir -p $(DESTDIR)
	mkdir -p $(DESTDIR)/tests/
	mkdir -p $(DESTDIR)/tcg/tpm/
	mkdir -p $(DESTDIR)/tcg/tspi/
	mkdir -p $(DESTDIR)/tcg/init/
	find tests -type f -perm /u+x \
		-exec install -v {} $(DESTDIR)tests/ \;
	find tcg/tpm/ -type f -perm /u+x \
		-exec install -v {} $(DESTDIR)tcg/tpm/ \;
	find tcg/tspi/ -type f -perm /u+x \
		-exec install -v {} $(DESTDIR)tcg/tspi/ \;
	find tcg/init/ -type f -perm /u+x \
		-exec install -v {} $(DESTDIR)tcg/init/ \;
	find tcg/highlevel/tpm/ -type f -perm /u+x \
		-exec install -v {} $(DESTDIR)tcg/tpm/ \;
	install -v tpmts.sh $(DESTDIR)
	install -v tpmts.man $(DESTDIR)

uninstall:
	@if [ $(DESTDIR) != "/" ]; then rm -rf $(DESTDIR); fi

tarball: clean
	tar -cpf ../tpmts-pkg.tar --exclude .svn \
		$(SUBDIRS) $(DEPDIR) $(INDDIR) $(DISTFILES)

dist: tarball
	mkdir tpmts-$(TPMTSVER)
	cd tpmts-$(TPMTSVER) && tar xpf ../../tpmts-pkg.tar
	tar cpf tpmts-$(TPMTSVER).tar tpmts-$(TPMTSVER)
	gzip -f tpmts-$(TPMTSVER).tar 
	$(RM) -r tpmts-$(TPMTSVER)
