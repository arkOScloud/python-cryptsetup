%{!?python_sitelib: %define python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib()")}
#I don't want the unpackaged file check
%define _unpackaged_files_terminate_build 0

Name:           pycryptsetup
Version:        0.0.1
Release:        1%{?dist}
Summary:        Python bindings for cryptsetup

Group:          Applications/System
License:        GPLv2+
URL:            http://fedorahosted.org/firstaidkit
Source0:        %{name}-%{version}.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  cryptsetup-luks-devel
BuildRequires:  python-devel
BuildRequires:  python-setuptools-devel

%description
A python module to ease the manipulation with LUKS devices.

%prep
%setup -q

%build
%{__python} setup.py build

%install
%{__rm} -rf $RPM_BUILD_ROOT
%{__python} setup.py install -O1 --skip-build --root $RPM_BUILD_ROOT

#docs
%{__install} -d $RPM_BUILD_ROOT%{_datadir}/doc/%name-%version
%{__install} -p COPYING $RPM_BUILD_ROOT%{_datadir}/doc/%name-%version/COPYING

#examples
%{__install} -d $RPM_BUILD_ROOT%{_libdir}/pycryptsetup/examples
%{__mv} -f selftest.py $RPM_BUILD_ROOT%{_libdir}/pycryptsetup/examples

%clean
%{__rm} -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
# For noarch packages: sitelib
%{python_sitelib}/pycryptsetup
%{python_sitelib}/cryptsetup.so
%{python_sitelib}/%{name}-%{version}-py2.5.egg-info
%attr(0644,root,root) %{_datadir}/doc/%name-%version/COPYING

