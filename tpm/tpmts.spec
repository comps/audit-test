Name:      tpmts
Version:   0.3
Release:   1
Summary:   Hardware enablement tests of the TPM
Packager:  Matt Anderson <mra@hp.com>
URL:       http://free.linux.hp.com/~mra/rpms/%{name}
Source0:   http://free.linux.hp.com/~mra/rpms/%{name}/%{name}-%{version}.tar.gz
License:   GPL v2
# Portions of this package are GPLv2 while others are yet to be licensed.
# For the purposes of meeting RPMs License checking this field was left GPLv2
Group:     Development/Testing
Requires:  trousers
BuildRequires:  trousers-devel
Prefix:    /usr/local/bin
BuildRoot: %{_tmppath}/%{name}-root

%description
This package is a self contained testing package to verify that a TPM has been
successfully enabled on a platform.  Included in the package are TrouSerS tests
along with a wrapper script which enables users to quickly verify that the
system is able to properly access and utilize the TPM.

%prep
%setup -q

%build
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/usr/local/tpmts/

%clean
make clean

%files
%defattr(-,root,root)
/usr/local/tpmts/
%doc /usr/local/tpmts/tpmts.man
