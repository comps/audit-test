Name:           audit-test
Version:        1090
Release:        1
Summary:        Audit testsuite for RHEL4 CAPP evaluation
Vendor:         Hewlett-Packard
Packager:       Aron Griffis <aron@hp.com>
Source0:        %{name}-%{version}.tar.gz
License:        GPL v2
Group:          Development/Tests
BuildArch:      noarch
Requires:       audit-libs-devel binutils cpp expect flex gcc gcc-c++ glibc-devel libattr-devel libstdc++-devel make
Prefix:         /usr/local/eal3_testing
BuildRoot:      %{_tmppath}/%{name}-root

%description
This is a test suite intended to aid CAPP certification of HP systems running
RHEL4 U2.  It was originally written by IBM engineers for certification of SLES
systems, then it was torn apart and mostly rewritten by HP engineers.

%prep
%setup -q -n %{name}

%build

%install
[ "$RPM_BUILD_ROOT" != / ] && rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{prefix}/%{name}
cp -a . $RPM_BUILD_ROOT/%{prefix}/%{name}

%clean
[ "$RPM_BUILD_ROOT" != / ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{prefix}/%{name}
