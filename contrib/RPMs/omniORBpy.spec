# omniORBpy RPM SPEC file

%define version       2.2pre1
%define release       1
%define name          omniORBpy
%define lib_major     2
%define lib_name      lib%{name}%{lib_major}
%define py_ver        %(python -c 'import sys;print(sys.version[0:3])')
%define prefix        /usr

Summary: Python Language Mapping for omniORB
Name:           %{name}
Version:        %{version}
Release:        %{release}
License:        GPL / LGPL
Group:          System/Libraries
Source0:        %{name}-%{version}.tar.gz
#Patch0:         omniORBpy.patches
URL:            http://omniorb.sourceforge.net/
Requires:       omniORB = 4.0.2pre1
Buildroot:      %{_tmppath}/%{name}-%{version}-root
BuildRequires:	omniORB-devel
#BuildArch:      i586

%description
omniORBpy is a Python language mapping for the omniORB CORBA
Object Request Broker (ORB).

# devel part of the bundle

%package -n %{name}-devel
Summary: Header files and libraries needed for %{name} development
Group:          Development/Python
Requires:       %{name} = %{version}-%{release} omniORB-devel
Provides:       libomniorbpy-devel = %{version}-%{release} %{name}-devel = %{version}-%{release}
#BuildArch:      i586

%description -n %{name}-devel
This package includes the header files and libraries needed for
developing programs using %{name}.

# docs and examples are in a separate package

%package -n %{name}-doc
Summary: Documentation needed for %{name} development
Group:          Development/Python
#Requires:       %{name} = %{version}
#BuildArch:      noarch

%description -n %{name}-doc
This package includes developer documentation including examples.

%prep 

%setup -n %{name}-%{version}
#%patch0 -p1

# Needs to know where omniORB was installed if it is not in /usr.
# If necessary, use the configure option --with-omniorb=%{prefix}
./configure --prefix=%{prefix} --with-openssl=/usr

%build
make CCFLAGS+="$RPM_OPT_FLAGS" all

%install
make DESTDIR=$RPM_BUILD_ROOT install

%clean
[ -z $RPM_BUILD_ROOT ] || rm -rf $RPM_BUILD_ROOT

%post -n %{name} -p /sbin/ldconfig

%postun -n %{name} -p /sbin/ldconfig

# main package includes libraries and servers
%files
%defattr (-,root,root)
%doc COPYING.LIB
%doc bugfixes*
%prefix/lib/python%{py_ver}/site-packages/_omni*.so*
%prefix/lib/python%{py_ver}/site-packages/*.py*
%prefix/lib/python%{py_ver}/site-packages/CosNaming/*.py*
%prefix/lib/python%{py_ver}/site-packages/CosNaming__POA/*.py*
%prefix/lib/python%{py_ver}/site-packages/omniORB.pth
%prefix/lib/python%{py_ver}/site-packages/omniORB

%files -n %{name}-devel
%defattr(-,root,root)
%doc README* ReleaseNotes* update.log
%prefix/include/omniORBpy.h
%prefix/include/omniORB4/pydistdate.hh
%prefix/lib/python%{py_ver}/site-packages/omniidl_be/python.py*

%files -n %{name}-doc
%defattr(-,root,root)
%doc doc/* 
%doc examples

%changelog
* Tue Jun 10 2003 Duncan Grisby <dgrisby@apasphere.com> 2.2pre1
- Fix some text, bump version number, minor tweaks.

* Wed Apr 23 2003 Sander Steffann <sander@steffann.nl>
- Included SSL support
- Included pydistdate.hh to stop RPM from complaining

* Mon Mar 17 2003 Thomas Lockhart <lockhart@fourpalms.org> 4.0.1
- Update for current release
- Include new omniORB.pth

* Mon Jul 29 2002 Thomas Lockhart <lockhart@fourpalms.org> 4.0.0beta
- Include backend to omniidl to allow generation of Python stubs

* Fri May 24 2002 Thomas Lockhart <lockhart@fourpalms.org> 4.0.0beta
- Use autoconf as available in this new version of omniORBpy
- Assume a separate omniORB RPM has been built and installed
- Based on the single RPM for omniORB and omniORBpy from the 3.x series
