
# Version number
OMNIPY_MAJOR = 0
OMNIPY_MINOR = 2

CXXSRCS = omnipy.cc \
          pyExceptions.cc \
          pyObjectRef.cc \
          pyMarshal.cc \
          pyTypeCode.cc \
          pyProxyCallWrapper.cc \
          pyServant.cc

OBJS    = omnipy.o \
          pyExceptions.o \
          pyObjectRef.o \
          pyMarshal.o \
          pyTypeCode.o \
          pyProxyCallWrapper.o \
          pyServant.o

DIR_CPPFLAGS = $(patsubst %,-I%/src/lib/omniORB2/orbcore,$(IMPORT_TREES))


#############################################################################
#   Make variables for Unix platforms                                       #
#############################################################################

ifdef UnixPlatform
CXXDEBUGFLAGS = -g

PYPREFIX := $(shell python -c 'import sys; print sys.prefix')
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
	$(CXXLINK) $(CXXLINKOPTIONS) -shared -o $@ -Wl,-soname,$(soname) $(IMPORT_LIBRARY_FLAGS) $(OMNIORB2_LIB_NODYN_DEPEND)\
	 $(filter-out $(LibSuffixPattern),$^) $(OMNIORB2_LIB_NODYN)\
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
         $(filter-out $(LibSuffixPattern),$^) $(OMNIORB2_LIB_NODYN) \
         $$CXX_RUNTIME \
	)

endif

ifeq ($(notdir $(CXX)),g++)

CXXOPTIONS += -fPIC

$(lib): $(OBJS)
	(set -x; \
	$(RM) $@; \
	$(CXXLINK) $(CXXLINKOPTIONS) -shared -o $@ -Wl-soname,$(soname) $(IMPORT_LIBRARY_FLAGS) $(OMNIORB2_LIB_NODYN_DEPEND)\
	 $(filter-out $(LibSuffixPattern),$^) $(OMNIORB2_LIB_NODYN)\
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
	 libs="$(OMNIORB2_LIB) python15.lib"; \
	 $(CXXLINK) -out:$@ -DLL $(CXXLINKOPTIONS) $(IMPORT_LIBRARY_FLAGS) $(PYLIBPATH) $(OBJS) $$libs; \
	)

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
	     -lomnithread2 -lomniORB28 \
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
T_LIBRARY_FLAGS) $(OMNIORB2_LIB_NODYN_DEPEND)\
        $(filter-out $(LibSuffixPattern),$^) $(OMNIORB2_LIB)\
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
