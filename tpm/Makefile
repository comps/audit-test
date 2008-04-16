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
	tar -cpf tpmts-pkg.tar testlib tpmts.sh
