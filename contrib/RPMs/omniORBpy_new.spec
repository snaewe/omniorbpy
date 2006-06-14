%define _name omniORBpy

%define lib_name %{?mklibname:%mklibname %{_name} 3}%{!?mklibname:lib%{_name}3}

Summary:   Python Language Mapping for omniORB
Name:      %{_name}
Version:   3.0
Release:   0.1
License:   GPL / LGPL
Group:     System/Libraries
Source0:   %{_name}-%{version}.tar.gz
Prefix:    /usr
URL:       http://omniorb.sourceforge.net/
BuildRequires: libomniorb-devel python python-devel
Buildroot: %{_tmppath}/%{name}-%{version}-root

%description
%{_name} is a Python language mapping for the omniORB CORBA
Object Request Broker (ORB).

%package -n %{lib_name}
Summary:   Python Language Mapping for omniORB
Group:     System/Libraries
Requires:  libomniorb
Provides:  libomniorbpy = %{version}-%{release} %{_name} = %{version}-%{release}
Conflicts: libomniorbpy < %{version}-%{release}
Obsoletes: omniORBpy libomniorbpy2

%description -n %{lib_name}
%{_name} is a Python language mapping for the omniORB CORBA
Object Request Broker (ORB).

# "standard" part of the bundle

%package standard
Summary:   Files to provide standard top-level CORBA module for %{_name}
Group:     Development/Python
Provides:  libomniorbpy-standard = %{version}-%{release} %{_name}-standard = %{version}-%{release}
Conflicts: libomniorbpy-standard < %{version}-%{release}
Obsoletes: omniORBpy-standard

%description standard
The CORBA to Python mapping standard requires top-level CORBA and
PortableServer modules. This provides those standard modules for
%{_name}. It will clash with similar packages for other Python ORBs.

# devel part of the bundle

%package -n %{lib_name}-devel
Summary:   Header files and libraries needed for %{_name} development
Group:     Development/Python
Requires:  %{lib_name} = %{version}-%{release} libomniorb-devel
Provides:  libomniorbpy-devel = %{version}-%{release} %{_name}-devel = %{version}-%{release}
Conflicts: libomniorbpy-devel < %{version}-%{release}
Obsoletes: omniORBpy-devel libomniorbpy2-devel

%description -n %{lib_name}-devel
The header files and libraries needed for developing programs using %{_name}.

# docs and examples are in a separate package

%package doc
Summary:   Documentation needed for %{_name} development
Group:     Development/Python
Obsoletes: omniORBpy-doc libomniorbpy-doc

%description doc
Developer documentation and examples.


%define py_ver %(python -c 'import sys;print(sys.version[0:3])')

%prep 

%setup -n %{_name}-%{version}

# Needs to know where omniORB was installed if it is not in /usr.
# If necessary, use the configure option --with-omniorb=%{_prefix}
%{?configure:%configure}%{!?configure:./configure --prefix=%{_prefix} --libdir=%{_libdir}} --with-openssl=%{_prefix}


%build
# We abuse the CPPFLAGS to pass optimisation options through.
%{?make:%make}%{!?make:make IMPORT_CPPFLAGS+="$RPM_OPT_FLAGS"} all

%install
[ -z %{buildroot} ] || rm -rf %{buildroot}

%{?makeinstall_std:%makeinstall_std}%{!?makeinstall_std:make DESTDIR=%{buildroot} install}

# omit omniidl_be/__init__.py because it is a duplicate of the file
# already provided by omniORB.
rm -rf %{buildroot}%{_libdir}/python%{py_ver}/site-packages/omniidl_be/__init__.py*


%clean
[ -z %{buildroot} ] || rm -rf %{buildroot}

%post -n %{lib_name} -p /sbin/ldconfig

%postun -n %{lib_name} -p /sbin/ldconfig


# main package includes libraries and servers
%files -n %{lib_name}
%defattr (-,root,root)
%doc COPYING.LIB
#%doc bugfixes*
%{_libdir}/python%{py_ver}/site-packages/_omni*.so.*
%{_libdir}/python%{py_ver}/site-packages/omniORB

%files standard
%defattr(-,root,root)
%{_libdir}/python%{py_ver}/site-packages/*.py*
%{_libdir}/python%{py_ver}/site-packages/omniORB.pth
%{_libdir}/python%{py_ver}/site-packages/CosNaming
%{_libdir}/python%{py_ver}/site-packages/CosNaming__POA

%files -n %{lib_name}-devel
%defattr(-,root,root)
%doc README* update.log
%{_includedir}/omniORBpy.h
%{_includedir}/omniORB4/pydistdate.hh
%{_libdir}/python%{py_ver}/site-packages/_omni*.so
%{_libdir}/python%{py_ver}/site-packages/omniidl_be/python.py*

%files doc
%defattr(-,root,root)
%doc doc/* 

%changelog
* Thu May 05 2005 Dirk Siebnich <dok@dok-net.net> 3.0-1
- rework package names to support x86_64

* Mon Jul 26 2004 Duncan Grisby <duncan@grisby.org> 2.4
- Bump version number to 2.4.

* Wed Nov 19 2003 Duncan Grisby <duncan@grisby.org> 2.3
- Merge contributed updates, bump version number.

* Fri Aug 08 2003 Thomas Lockhart <lockhart@fourpalms.org>
- Drop circular definition of RPM macros

* Wed Aug  6 2003 Duncan Grisby <dgrisby@apasphere.com> 2.2
- Remove clashing omniidl __init__.py. Bump version number.

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
