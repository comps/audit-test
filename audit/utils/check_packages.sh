#!/bin/sh

RC=0

ARCH=`uname -m`

X="i486 i586 i686 ix86"
P="ppc powerpc"
IP="ppc64 powerpc64"
Z="s390"
X86_64="x86_64"
IA="ia64"

APPS="attr-devel \
binutils \
cpp \
expect \
perl-Expect \
flex \
gcc \
gcc-c++ \
glibc-devel \
kernel-source \
laus-devel \
libstdc++-devel \
make \
openssl-devel"
 
PPC64_APPS="cross-ppc64-gcc \
cross-ppc64-libs_and_headers"

if [ `echo $P $IP | grep $ARCH | wc -l` -gt 0 ]; then
	APPS="$APPS $PPC64_APPS"
fi

for app in $APPS 
do
  out=`rpm -q $app 2>&1`
  if [ $? -eq 0 ]; then
      echo "[$app] -> OK"
  else
      echo "[$app] -> <<< NOT INSTALLED >>>"
      RC=1
  fi
done

exit $RC
