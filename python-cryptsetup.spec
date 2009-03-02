%{!?python_sitearch: %define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}

Name:           python-cryptsetup
Version:        0.0.4
Release:        1%{?dist}
Summary:        Python bindings for cryptsetup

Group:          Development/Libraries
License:        GPLv2+
Url:            http://msivak.fedorapeople.org/pycryptsetup
Source:         http://msivak.fedorapeople.org/pycryptsetup/%{name}-%{version}.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  cryptsetup-luks-devel
BuildRequires:  python
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
%{python_sitearch}/*egg-info

%doc COPYING
%doc selftest.py

%changelog
* Mon Mar 01 2009 Martin Sivak <msivak at redhat dot com> - 0.0.4-1
- add prepare_passphrase_file method

* Mon Mar 01 2009 Martin Sivak <msivak at redhat dot com> - 0.0.3-1
- Improve documentation
- luksFormat now accepts keyfile argument

* Mon Feb 23 2009 Martin Sivak <msivak at redhat dot com> - 0.0.2-1
- Throw a runtime exception when buildvalue problem is encountered

* Thu Jan 22 2009 Martin Sivak <msivak at redhat dot com> - 0.0.1-1
- Inital release

