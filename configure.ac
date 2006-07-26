dnl -*- M4 -*-
AC_INIT(omniORBpy, 3.0, bugs@omniorb-support.com)

AC_PREREQ(2.52)
AC_CONFIG_SRCDIR(python/omniORB/CORBA.py)
AC_CONFIG_AUX_DIR(bin/scripts)

AC_CANONICAL_BUILD
AC_CANONICAL_HOST


dnl ** Programs

AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AM_PATH_PYTHON
AC_LANG(C++)


dnl ** Libraries

OMNI_OMNIORB_ROOT
OMNI_OPENSSL_ROOT


dnl ** More programs

abssrcdir=`cd $srcdir; pwd`
searchpath=$abssrcdir/bin/scripts:$OMNIORB_ROOT/bin:/usr/local/bin:/usr/bin:$PATH

AC_PATH_PROGS(OMKDEPEND, omkdepend, omkdepend,$searchpath)
AC_PATH_PROGS(MKDIRHIER, omkdirhier mkdirhier, omkdirhier, $searchpath)


dnl ** Compiler name

compiler_name="Compiler_Unknown"

if test "$ac_cv_cxx_compiler_gnu" = "yes"; then
  compiler_name="Compiler_GCC"
elif test `basename $CXX` = "KCC"; then
  compiler_name="Compiler_KCC"
else
  case "$host" in
  *-*-solaris*)
    if $CXX -V 2>&1 | grep '5\.[[0-9]]' >/dev/null 2>&1; then
      compiler_name="Compiler_Sun5"
    elif $CXX -V 2>&1 | grep '4\.[[0-9]]' >/dev/null 2>&1; then
      compiler_name="Compiler_Sun4"
    fi
  ;;
  *-*-osf*)
    # *UNK* We need to know how to ask DEC CXX what version it is
    compiler_name="Compiler_DEC61"
  ;;
  *-*-hpux*)
    compiler_name="Compiler_aCC"
  ;;
  *-*-irix*)
    compiler_name="Compiler_SGI"
  ;;
  *-*-irix*)
    compiler_name="Compiler_xlC"
  ;;
  esac
fi

AC_SUBST(COMPILER_NAME, $compiler_name)


dnl ** Platform name

platform_name="Platform_Unknown"
os_version="0"

case "$host" in
  *-*-linux-*)   platform_name="Linux";    os_version="2";;
  *-*-solaris*)  platform_name="SunOS";    os_version="5";;
  *-*-osf3*)     platform_name="OSF1";     os_version="3";;
  *-*-osf4*)     platform_name="OSF1";     os_version="4";;
  *-*-osf5*)     platform_name="OSF1";     os_version="5";;
  *-*-hpux10*)   platform_name="HPUX";     os_version="10";;
  *-*-hpux11*)   platform_name="HPUX";     os_version="11";;
  *-*-nextstep*) platform_name="NextStep"; os_version="3";;
  *-*-openstep*) platform_name="NextStep"; os_version="3";;
  *-*-irix*)     platform_name="IRIX";     os_version="6";;
  *-*-aix*)      platform_name="AIX";      os_version="4";;
  *-*-darwin*)   platform_name="Darwin";   os_version="1";;
  *-*-freebsd3*) platform_name="FreeBSD";  os_version="3";;
  *-*-freebsd4*) platform_name="FreeBSD";  os_version="4";;
  *-*-freebsd5*) platform_name="FreeBSD";  os_version="5";;
  *-*-sco*)      platform_name="OSR5";     os_version="5";;
  *-*-cygwin*)   platform_name="Cygwin";   os_version="2";;
esac

AC_SUBST(PLATFORM_NAME, $platform_name)
AC_SUBST(OSVERSION, $os_version)


dnl ** Processor

processor_name="UnknownProcessor"

case "$host" in
  i?86-*)   processor_name="x86Processor";;
  sparc-*)  processor_name="SparcProcessor";;
  alpha*)   processor_name="AlphaProcessor";;
  m68k-*)   processor_name="m68kProcessor";;
  mips*)    processor_name="IndigoProcessor";;
  arm-*)    processor_name="ArmProcessor";;
  s390-*)   processor_name="s390Processor";;
  ia64-*)   processor_name="ia86Processor";;
  hppa*)    processor_name="HppaProcessor";;
  powerpc*) processor_name="PowerPCProcessor";;
esac

AC_SUBST(PROCESSOR_NAME, $processor_name)


dnl ** Output

AC_CONFIG_FILES(mk/beforeauto.mk mk/afterauto.mk)

AC_CONFIG_FILES(
                GNUmakefile
                examples/embed/GNUmakefile
                modules/GNUmakefile
                modules/codesets/GNUmakefile
                modules/sslTP/GNUmakefile
                modules/connections/GNUmakefile
                omniidl_be/GNUmakefile
                python/GNUmakefile
                python/COS/GNUmakefile
                python/omniORB/GNUmakefile
		include/GNUmakefile
               )

AC_OUTPUT
