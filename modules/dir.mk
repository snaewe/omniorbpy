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


CXXDEBUGFLAGS = -g


ifdef Linux

CXXOPTIONS += -fpic

libname = _omnipymodule.so
soname  = $(libname).0
lib     = $(soname).0

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


ifdef SunOS

CXXOPTIONS += -Kpic -I/usr/local/include

libname = _omnipymodule.so
soname  = $(libname).0
lib     = $(soname).0

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

