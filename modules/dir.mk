# Version number
OMNIPY_MAJOR = 0
OMNIPY_MINOR = 5

ifeq ($(OMNIORB_VERSION),)
OMNIORB_VERSION = 2.8.0
OMNIORB_MAJOR_VERSION = 2
OMNIORB_MINOR_VERSION = 8
endif

DIR_CPPFLAGS += -DOMNIPY_MAJOR=$(OMNIPY_MAJOR) -DOMNIPY_MINOR=$(OMNIPY_MINOR)
DIR_CPPFLAGS += -DOMNIORB_VERSION_STRING=\"$(OMNIORB_VERSION)\"

OMNIORB_VERSION_NOMICRO := $(OMNIORB_MAJOR_VERSION).$(OMNIORB_MINOR_VERSION)

# Test which omniORB version we're using

ifeq ($(OMNIORB_VERSION_NOMICRO), 2.8)
VERSIONDIR = omni28
DIR_CPPFLAGS += -DOMNIORBPY_FOR_28

OMNIORB_LIB              = $(OMNIORB2_LIB)
OMNIORB_LIB_NODYN        = $(OMNIORB2_LIB_NODYN)
OMNIORB_LIB_NODYN_DEPEND = $(OMNIORB2_LIB_NODYN_DEPEND)
endif

ifeq ($(OMNIORB_VERSION_NOMICRO), 3.1)
VERSIONDIR = omni30
DIR_CPPFLAGS += -DOMNIORBPY_FOR_30
endif

ifeq ($(OMNIORB_VERSION_NOMICRO), 3.0)
VERSIONDIR = omni30
DIR_CPPFLAGS += -DOMNIORBPY_FOR_30
endif


SUBDIRS = $(VERSIONDIR) common
all::
	@$(MakeSubdirs)
export::
	@$(MakeSubdirs)


CXXSRCS = $(VERSIONDIR)/omnipy.cc \
          $(VERSIONDIR)/pyORBFunc.cc \
          $(VERSIONDIR)/pyPOAFunc.cc \
          $(VERSIONDIR)/pyPOAManagerFunc.cc \
          $(VERSIONDIR)/pyObjectRef.cc \
          $(VERSIONDIR)/pyCallDescriptor.cc \
          $(VERSIONDIR)/pyServant.cc \
          common/pyExceptions.cc \
          common/pyMarshal.cc \
          common/pyTypeCode.cc \
          common/pyThreadCache.cc \
          common/pyomniFunc.cc

OBJS =    $(VERSIONDIR)/omnipy.o \
          $(VERSIONDIR)/pyORBFunc.o \
          $(VERSIONDIR)/pyPOAFunc.o \
          $(VERSIONDIR)/pyPOAManagerFunc.o \
          $(VERSIONDIR)/pyObjectRef.o \
          $(VERSIONDIR)/pyCallDescriptor.o \
          $(VERSIONDIR)/pyServant.o \
          common/pyExceptions.o \
          common/pyMarshal.o \
          common/pyTypeCode.o \
          common/pyThreadCache.o \
          common/pyomniFunc.o



DIR_CPPFLAGS += $(patsubst %,-I%/src/lib/omniORB2,$(IMPORT_TREES))
DIR_CPPFLAGS += $(patsubst %,-I%/src/lib/omniORB2/orbcore,$(IMPORT_TREES))


#############################################################################
#   Make variables for Unix platforms                                       #
#############################################################################

ifdef UnixPlatform
#CXXDEBUGFLAGS = -g

PYPREFIX  := $(shell $(PYTHON) -c 'import sys; print sys.exec_prefix')
PYVERSION := $(shell $(PYTHON) -c 'import sys; print sys.version[:3]')
PYINCDIR  := $(PYPREFIX)/include
PYINCFILE := "<python$(PYVERSION)/Python.h>"
PYINCTHRD := "<python$(PYVERSION)/pythread.h>"
DIR_CPPFLAGS += -I$(PYINCDIR) -DPYTHON_INCLUDE=$(PYINCFILE) -DPYTHON_THREAD_INC=$(PYINCTHRD)

endif

#############################################################################
#   Make rules for Linux                                                    #
#############################################################################

ifdef Linux

CXXOPTIONS += -fpic

libname = _omnipymodule.so
soname  = $(libname).$(OMNIPY_MAJOR)
lib     = $(soname).$(OMNIPY_MINOR)

$(lib): $(OBJS)
	(set -x; \
	$(RM) $@; \
	$(CXXLINK) $(CXXLINKOPTIONS) -shared -o $@ -Wl,-soname,$(soname) $(IMPORT_LIBRARY_FLAGS) \
	 $(filter-out $(LibSuffixPattern),$^) $(OMNIORB_LIB_NODYN)\
	)

all:: $(lib)

clean::
	$(RM) $(lib)

export:: $(lib)
	@$(ExportLibrary)
	@(set -x; \
          cd $(EXPORT_TREE)/$(LIBDIR); \
          $(RM) $(soname); \
          ln -s $(lib) $(soname); \
          $(RM) $(libname); \
          ln -s $(soname) $(libname); \
         )
endif


#############################################################################
#   Make rules for Solaris 2.x                                              #
#############################################################################

ifdef SunOS

libname = _omnipymodule.so
soname  = $(libname).$(OMNIPY_MAJOR)
lib     = $(soname).$(OMNIPY_MINOR)

ifeq ($(notdir $(CXX)),CC)

CXXOPTIONS += -Kpic

$(lib): $(OBJS)
	(set -x; \
	$(RM) $@; \
	if (CC -V 2>&1 | grep '5\.[0-9]'); \
	  then CXX_RUNTIME=-lCrun; \
	  else CXX_RUNTIME=-lC; \
        fi; \
        $(CXX) -G -o $@ -h $(soname) $(IMPORT_LIBRARY_FLAGS) \
         $(patsubst %,-R %,$(IMPORT_LIBRARY_DIRS)) \
         $(filter-out $(LibSuffixPattern),$^) $(OMNIORB_LIB_NODYN) \
         $$CXX_RUNTIME \
	)

endif

ifeq ($(notdir $(CXX)),g++)

CXXOPTIONS += -fPIC

$(lib): $(OBJS)
	(set -x; \
	$(RM) $@; \
	$(CXXLINK) $(CXXLINKOPTIONS) -shared -o $@ -Wl-soname,$(soname) $(IMPORT_LIBRARY_FLAGS) \
	 $(filter-out $(LibSuffixPattern),$^) $(OMNIORB_LIB_NODYN)\
	)

endif

all:: $(lib)

clean::
	$(RM) $(lib)

export:: $(lib)
	@$(ExportLibrary)
	@(set -x; \
          cd $(EXPORT_TREE)/$(LIBDIR); \
          $(RM) $(soname); \
          ln -s $(lib) $(soname); \
          $(RM) $(libname); \
          ln -s $(soname) $(libname); \
         )

endif


#############################################################################
#   Make rules for Windows                                                  #
#############################################################################

ifdef Win32Platform

PYPREFIX1 := "$(shell $(PYTHON) -c 'import sys,string; sys.stdout.write(string.lower(sys.prefix))')"
PYPREFIX  := $(subst program files,progra~1,$(subst \,/,$(PYPREFIX1)))
PYVERSION := $(shell $(PYTHON) -c 'import sys; sys.stdout.write(sys.version[:3])')
PYINCDIR  := $(PYPREFIX)/include
PYLIBDIR  := $(PYPREFIX)/libs
PYLIB     := python$(subst .,,$(PYVERSION)).lib

DIR_CPPFLAGS += -I$(PYINCDIR) -I$(PYINCDIR)/python$(PYVERSION) \
                -DPYTHON_INCLUDE="<Python.h>" -DPYTHON_THREAD_INC="<pythread.h>"

PYLIBPATH = $(patsubst %,-libpath:%,$(PYLIBDIR))

implib = _omnipy.lib
lib = $(patsubst %.lib,%.pyd,$(implib))

all:: $(lib)

ifeq ($(OMNIORB_VERSION),2.8.0)

$(lib): $(OBJS)
	(if egrep '^ *static *omni_mutex *objectTableLock' $(TOP)/include/omniORB2/omniInternal.h; then \
           echo -e '\n\n\n\a'; \
	   echo '*** ERROR !!! ***'; \
	   echo; \
	   echo 'Your $$TOP\include\omniORB2\omniInternal.h file needs to be patched.'; \
	   echo; \
           echo 'Please check-out the latest omni2_8_develop branch from CVS.'; \
           echo 'See http://www.uk.research.att.com/omniORB/cvs.html for details.'; \
           echo; \
           exit 1; \
         fi; \
	 set -x; \
	 $(RM) $@; \
	 libs="$(OMNIORB_LIB) $(PYLIB)"; \
	 $(CXXLINK) -out:$@ -DLL $(CXXLINKOPTIONS) $(IMPORT_LIBRARY_FLAGS) $(PYLIBPATH) $(OBJS) $$libs; \
	)

else

$(lib): $(OBJS)
	(set -x; \
	 $(RM) $@; \
	 libs="$(OMNIORB_LIB) $(PYLIB)"; \
	 $(CXXLINK) -out:$@ -DLL $(CXXLINKOPTIONS) $(IMPORT_LIBRARY_FLAGS) $(PYLIBPATH) $(OBJS) $$libs; \
	)
endif

export:: $(lib)
	@$(ExportLibrary)


endif


#############################################################################
#   Make rules for AIX                                                      #
#############################################################################

ifdef AIX

CXXOPTIONS += -I. -I/usr/local/include

DIR_CPPFLAGS += $(CORBA_CPPFLAGS)

lib = _omnipymodule.so
libinit = init_omnipy
py_exp = /usr/local/lib/python$(PYVERSION)/config/python.exp

ifeq ($(notdir $(CXX)),xlC_r)

$(lib): $(OBJS) $(PYOBJS)
	@(set -x; \
	$(RM) $@; \
	$(MAKECPPSHAREDLIB) \
	     -o $(lib) \
	     -bI:$(py_exp) \
	     -n $(libinit) \
	     $(IMPORT_LIBRARY_FLAGS) \
	     $(OMNIORB_LIB_NODYN) \
	     -bhalt:4 -T512 -H512 \
	     $(filter-out $(LibSuffixPattern),$^) \
	     -p 40 \
	 ; \
       )

endif

all:: $(lib)

clean::
	$(RM) $(lib)

export:: $(lib)
	@$(ExportLibrary)

endif



#############################################################################
#   Make rules for FreeBSD                                                    #
#############################################################################

ifdef FreeBSD

CXXOPTIONS += -fPIC

libname = _omnipymodule.so
soname  = $(libname).$(OMNIPY_MAJOR)
lib     = $(soname).$(OMNIPY_MINOR)

$(lib): $(OBJS)
	(set -x; \
       $(RM) $@; \
       $(CXXLINK) $(CXXLINKOPTIONS) $(CORBA_LIB_NO_DYN) $(OMNITHREAD_LIB_NODYN) \
       -shared -o $@ -Wl,-soname,$(soname) \
       $(IMPORT_LIBRARY_FLAGS) \
        $(filter-out $(LibSuffixPattern),$^) $(OMNIORB_LIB)\
       )

all:: $(lib)

clean::
	$(RM) $(lib)

export:: $(lib)
	@$(ExportLibrary)
	@(set -x; \
          cd $(EXPORT_TREE)/$(LIBDIR); \
          $(RM) $(soname); \
          ln -s $(lib) $(soname); \
          $(RM) $(libname); \
          ln -s $(soname) $(libname); \
         )
endif


#############################################################################
#   Make rules for NextStep                                                 #
#############################################################################

ifdef NextStep

PYPREFIX = $(shell $(PYTHON) -c "import sys;print sys.exec_prefix")
CXXOPTIONS += -I$(PYPREFIX)/include
CXXLINKOPTIONS += -nostdlib -r
SO = .so
libname = _omnipymodule$(SO)
soname  = $(libname).$(OMNIPY_MAJOR)
lib     = $(soname).$(OMNIPY_MINOR)

$(lib): $(OBJS)
      $(CXXLINK) $(CXXLINKOPTIONS) $(OBJS) $(OMNIORB_LIB_NODYN) -o $(lib)

all:: $(lib)

clean::
      $(RM) $(lib)

export:: $(lib)
      @$(ExportLibrary)
      @(set -x; 
              cd $(EXPORT_TREE)/$(LIBDIR); 
              $(RM) $(soname); 
              ln -s $(lib) $(soname); 
              $(RM) $(libname); 
              ln -s $(soname) $(libname); 
      )
endif

#############################################################################
#   Make rules for HPUX                                                     #
#############################################################################

ifdef HPUX
ifeq ($(notdir $(CXX)),aCC)

# Note: the python installation must be built to load C++ shared library
#       this usually means that the main function of the python executable
#       is compiled and linked with aCC.

CXXOPTIONS += +Z

libname = _omnipymodule.sl
soname  = $(libname).$(OMNIPY_MAJOR)
lib     = $(soname).$(OMNIPY_MINOR)

$(lib): $(OBJS)
	(set -x; \
         $(RM) $@; \
         aCC -b -Wl,+h$(soname) -o $@  $(IMPORT_LIBRARY_FLAGS) \
           $(patsubst %,-L %,$(IMPORT_LIBRARY_DIRS)) \
           $(filter-out $(LibSuffixPattern),$^) $(OMNIORB_LIB_NODYN); \
        )

all:: $(lib)

clean::
	$(RM) $(lib)

export:: $(lib)
	@$(ExportLibrary)
	@(set -x; \
          cd $(EXPORT_TREE)/$(LIBDIR); \
          $(RM) $(soname); \
          ln -s $(lib) $(soname); \
          $(RM) $(libname); \
          ln -s $(soname) $(libname); \
         )

endif
endif

#############################################################################
#   Make rules for SGI Irix 6.2                                             #
#############################################################################

ifdef IRIX
ifeq ($(notdir $(CXX)),CC)

CXXOPTIONS += -KPIC

ifdef IRIX_n32
ADD_CPPFLAGS = -n32
endif
ifdef IRIX_64
ADD_CPPFLAGS = -64
endif

libname = _omnipymodule.so
soname  = $(libname).$(OMNIPY_MAJOR)
lib     = $(soname).$(OMNIPY_MINOR)

$(lib): $(OBJS)
	(set -x; \
         $(RM) $@; \
         $(LINK.cc) -KPIC -shared -Wl,-h,$(libname) \
           -Wl,-set_version,$(soname) -Wl,-rpath,$(LIBDIR) \
           -o $@ $(IMPORT_LIBRARY_FLAGS) \
           $(filter-out $(LibSuffixPattern),$^) $(OMNIORB_LIB_NODYN)\
           $(LDLIBS); \
        )

all:: $(lib)

clean::
	$(RM) $(lib)

export:: $(lib)
	@$(ExportLibrary)
	@(set -x; \
          cd $(EXPORT_TREE)/$(LIBDIR); \
          $(RM) $(soname); \
          ln -s $(lib) $(soname); \
          $(RM) $(libname); \
          ln -s $(soname) $(libname); \
         )

endif
endif


#############################################################################
#   Make rules for Digital Unix                                             #
#############################################################################

ifdef OSF1
ifeq ($(notdir $(CXX)),cxx)

libname = _omnipymodule.so
soname  = $(libname).$(OMNIPY_MAJOR)
lib     = $(soname).$(OMNIPY_MINOR)

all:: $(lib)

$(lib): $(OBJS)
	(set -x; \
         $(RM) $@; \
         ld -shared -soname $(soname) -set_version $(soname) -o $@ $(IMPORT_LIBRARY_FLAGS) \
         $(filter-out $(LibSuffixPattern),$^) $(OMNIORB_LIB_NODYN) -lcxxstd -lcxx -lexc -lots -lc \
        )


clean::
	$(RM) $(lib)

export:: $(lib)
	@$(ExportLibrary)
	@(set -x; \
          cd $(EXPORT_TREE)/$(LIBDIR); \
          $(RM) $(soname); \
          ln -s $(lib) $(soname); \
          $(RM) $(libname); \
          ln -s $(soname) $(libname); \
         )

endif
endif

#############################################################################
#   Make rules for Darwin                                                   #
#############################################################################

ifdef Darwin

CXXOPTIONS += $(SHAREDLIB_CPPFLAGS)

libname = _omnipymodule.so
soname  = _omnipymodule.$(OMNIPY_MAJOR).so
lib     = _omnipymodule.$(OMNIPY_MAJOR).$(OMNIPY_MINOR).so

$(lib): $(OBJS)
       (set -x; \
         $(RM) $@; \
         $(CXX) -bundle -undefined suppress -o $@ $(IMPORT_LIBRARY_FLAGS) \
         $(filter-out $(LibSuffixPattern),$^) $(OMNIORB_LIB_NODYN) \
       )

export:: $(lib)
       @$(ExportLibrary)
       @(set -x; \
               cd $(EXPORT_TREE)/$(LIBDIR); \
               $(RM) $(soname); \
               ln -s $(lib) $(soname); \
               $(RM) $(libname); \
               ln -s $(soname) $(libname); \
       )

all:: $(lib)

clean::
       $(RM) $(lib)

endif
