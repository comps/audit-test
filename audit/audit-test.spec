Name:           audit-test
Version:        2015
Release:        1
Summary:        Audit testsuite for RHEL5 CAPP/LSPP evaluation
Vendor:         Hewlett-Packard
Packager:       Aron Griffis <aron@hp.com>
Source0:        %{name}-%{version}.tar.gz
License:        GPL v2
Group:          Development/Tests
BuildArch:      noarch
Requires:       audit-libs-devel binutils cpp expect flex gcc gcc-c++ glibc-devel libattr-devel libstdc++-devel make libselinux-devel selinux-policy-devel
Prefix:         /usr/local/eal4_testing
BuildRoot:      %{_tmppath}/%{name}-root

%description
This is a test suite intended to aid CAPP/LSPP certification of HP systems
running RHEL5.

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
