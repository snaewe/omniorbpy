# Version number
OMNIPY_MAJOR = 0
OMNIPY_MINOR = 4

ifeq ($(OMNIORB_VERSION),)
OMNIORB_VERSION = 2.8.0
endif

DIR_CPPFLAGS += -DOMNIPY_MAJOR=$(OMNIPY_MAJOR) -DOMNIPY_MINOR=$(OMNIPY_MINOR)
DIR_CPPFLAGS += -DOMNIORB_VERSION_STRING=\"$(OMNIORB_VERSION)\"

ifeq ($(OMNIORB_MAJOR_VERSION).$(OMNIORB_MINOR_VERSION), 3.0)

VERSIONDIR = omni30
DIR_CPPFLAGS += -DOMNIORBPY_FOR_30

else

VERSIONDIR = omni28
DIR_CPPFLAGS += -DOMNIORBPY_FOR_28

OMNIORB_LIB              = $(OMNIORB2_LIB)
OMNIORB_LIB_NODYN        = $(OMNIORB2_LIB_NODYN)
OMNIORB_LIB_NODYN_DEPEND = $(OMNIORB2_LIB_NODYN_DEPEND)

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
          common/pyTypeCode.cc

OBJS =    $(VERSIONDIR)/omnipy.o \
          $(VERSIONDIR)/pyORBFunc.o \
          $(VERSIONDIR)/pyPOAFunc.o \
          $(VERSIONDIR)/pyPOAManagerFunc.o \
          $(VERSIONDIR)/pyObjectRef.o \
          $(VERSIONDIR)/pyCallDescriptor.o \
	  $(VERSIONDIR)/pyServant.o \
          common/pyExceptions.o \
          common/pyMarshal.o \
          common/pyTypeCode.o


DIR_CPPFLAGS += $(patsubst %,-I%/src/lib/omniORB2,$(IMPORT_TREES))
DIR_CPPFLAGS += $(patsubst %,-I%/src/lib/omniORB2/orbcore,$(IMPORT_TREES))


#############################################################################
#   Make variables for Unix platforms                                       #
#############################################################################

ifdef UnixPlatform
CXXDEBUGFLAGS = -g

PYPREFIX := $(shell python -c 'import sys; print sys.exec_prefix')
PYINCDIR := $(PYPREFIX)/include
DIR_CPPFLAGS += -I$(PYINCDIR)
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

PYPREFIX1 := $(shell python -c 'import sys; sys.stdout.write(sys.prefix)')
PYPREFIX := $(subst Program Files,progra~1,$(PYPREFIX1))
PYINCDIR := $(PYPREFIX)/include
PYLIBDIR := $(PYPREFIX)/libs

DIR_CPPFLAGS += -I"$(PYINCDIR)"
PYLIBPATH = -libpath:"$(PYLIBDIR)"

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
	 libs="$(OMNIORB_LIB) python15.lib"; \
	 $(CXXLINK) -out:$@ -DLL $(CXXLINKOPTIONS) $(IMPORT_LIBRARY_FLAGS) $(PYLIBPATH) $(OBJS) $$libs; \
	)

else

$(lib): $(OBJS)
	(set -x; \
	 $(RM) $@; \
	 libs="$(OMNIORB_LIB) python15.lib"; \
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
py_exp = /usr/local/lib/python1.5/config/python.exp

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
       $(CXXLINK) $(CXXLINKOPTIONS) -shared -o $@ -Wl,-soname,$(soname) $(IMPOR
T_LIBRARY_FLAGS) \
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

PYPREFIX = $(shell python -c "import sys;print sys.exec_prefix")
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
