Name:           audit-testsuite
Version:        576
Release:        1
Summary:        Audit testsuite for RHEL4 CAPP evaluation
Vendor:         Hewlett-Packard
Packager:       Aron Griffis <aron@hp.com>
Source0:        %{name}-%{version}.tar.gz
License:        GPL v2
Group:          Development/Tests
BuildArch:      noarch
Requires:       binutils cpp expect flex gcc gcc-c++ glibc-devel libattr-devel libstdc++-devel make
Prefix:         /root
BuildRoot:      %{_tmppath}/%{name}-root

%description
This is a test suite intended to aid CAPP certification of HP systems running
RHEL4 U2.  It was originally written by IBM engineers for certification of SLES
systems, then it was torn apart and mostly rewritten by HP engineers.

%prep
%setup -q

%build

%install
[ "$RPM_BUILD_ROOT" != / ] && rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/root/%{name}-%{version}
cp -a . $RPM_BUILD_ROOT/root/%{name}-%{version}

%clean
[ "$RPM_BUILD_ROOT" != / ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/root/%{name}-%{version}
