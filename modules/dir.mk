
# Version number
OMNIPY_MAJOR = 0
OMNIPY_MINOR = 1

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

DIR_CPPFLAGS = $(patsubst %,-I%/src/lib/omniORB2/orbcore,$(IMPORT_TREES)) \
               -DOMNIPY_MAJOR=$(OMNIPY_MAJOR) -DOMNIPY_MINOR=$(OMNIPY_MINOR)


#############################################################################
#   Make variables for Unix platforms                                       #
#############################################################################

ifdef UnixPlatform
CXXDEBUGFLAGS = -g
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
	$(CXXLINK) $(CXXLINKOPTIONS) -shared -o $@ -Wl,-soname,$(soname) $(IMPORT_LIBRARY_FLAGS) $(CORBA_LIB_DEPEND)\
	 $(filter-out $(LibSuffixPattern),$^) $(CORBA_LIB)\
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

CXXOPTIONS += -Kpic -I/usr/local/include

libname = _omnipymodule.so
soname  = $(libname).$(OMNIPY_MAJOR)
lib     = $(soname).$(OMNIPY_MINOR)

$(lib): $(OBJS)
	(set -x; \
	$(RM) $@; \
        $(CXX) -G -o $@ -h $(soname) $(IMPORT_LIBRARY_FLAGS) \
         $(patsubst %,-R %,$(IMPORT_LIBRARY_DIRS)) \
         $(filter-out $(LibSuffixPattern),$^) -lomnithread -lpthread -lposix4 -mt -lsocket -lnsl -lomniORB2 -lomniDynamic2 -ltcpwrapGK -lC \
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
#   Make rules for Windows                                                  #
#############################################################################

ifdef Win32Platform

DIR_CPPFLAGS += -I"c:\progra~1/Python/include"
PYLIBPATH = -libpath:"c:\progra~1\Python\libs"

implib = _omnipy.lib
lib = $(patsubst %.lib,%.pyd,$(implib))

all:: $(lib)

$(lib): $(OBJS)
	(if egrep '^ *static *omni_mutex *objectTableLock' $(OMNI_TREE)/include/omniORB2/omniInternal.h; then \
           echo -e '\n\n\n\a'; \
	   echo '*** ERROR !!! ***'; \
	   echo; \
	   echo 'Your $(OMNI_TREE)/include/omniORB2/omniInternal.h file needs to be patched.'; \
	   echo; \
           echo 'Please check-out the latest omni2_8_develop branch from CVS.'; \
           echo 'See http://www.uk.research.att.com/omniORB/cvs.html for details.'; \
           echo; \
           exit 1; \
         fi; \
	 set -x; \
	 $(RM) $@; \
	 libs="$(CORBA_LIB) python15.lib"; \
	 $(CXXLINK) -out:$@ -DLL $(CXXLINKOPTIONS) $(IMPORT_LIBRARY_FLAGS) $(PYLIBPATH) $(OBJS) $$libs; \
	)

export:: $(lib)
	@$(ExportLibrary)


endif
