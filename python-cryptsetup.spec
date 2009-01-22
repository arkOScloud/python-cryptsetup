%{!?python_sitelib: %define python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib()")}
%{!?python_sitearch: %define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}
#I don't want the unpackaged file check
%define _unpackaged_files_terminate_build 0

Name:           python-cryptsetup
Version:        0.0.1
Release:        1%{?dist}
Summary:        Python bindings for cryptsetup

Group:          Applications/System
License:        GPLv2+
URL:            http://fedorahosted.org/firstaidkit
Source0:        %{name}-%{version}.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

Requires: cryptsetup-luks
Requires: python

BuildRequires:  cryptsetup-luks-devel
BuildRequires:  python
BuildRequires:  python-devel
BuildRequires:  python-setuptools-devel

%description
A python module to ease the manipulation with LUKS devices.

%prep
%setup -q

%build
%{__python} setup.py build

%install
rm -rf $RPM_BUILD_ROOT
%{__python} setup.py install -O1 --skip-build --root $RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
# For noarch packages: sitelib
%{python_sitearch}/pycryptsetup
%{python_sitearch}/cryptsetup.so
%{python_sitearch}/%{name}-%{version}-py*.egg-info

%doc COPYING
%doc selftest.py

%changelog
* Thu Jan 22 2009 Martin Sivak <msivak at redhat dot com> - 0.0.1
- Inital release

