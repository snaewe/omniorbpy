PYLIBROOT= $(EXPORT_TREE)/lib/python
PYLIBDIR = $(PYLIBROOT)/omniORB
INSTALLPYLIBDIR = $(INSTALLPYTHONDIR)/omniORB

ir_idl.py: ir.idl
	$(OMNIIDL) -v -p$(BASE_OMNI_TREE)/omniidl_be \
        -I$(BASE_OMNI_TREE)/idl \
        -I$(OMNIORB_ROOT)/idl/omniORB \
        -I$(OMNIORB_ROOT)/share/idl/omniORB \
        -I$(DATADIR)/idl/omniORB \
        -bpython -Wbinline -Wbno_package $^

minorfile := $(shell file="$(INCDIR)/omniORB4/minorCode.h"; \
               dirs="$(IMPORT_TREES)"; \
               $(FindFileInDirs); \
               echo "$$fullfile")

minorCodes.py: $(minorfile)
	$(PYTHON) $(BASE_OMNI_TREE)/bin/scripts/makeminors.py $^ $@

all:: ir_idl.py minorCodes.py


FILES = __init__.py CORBA.py PortableServer.py PortableServer__POA.py \
        tcInternal.py URI.py codesets.py any.py BiDirPolicy.py \
        interceptors.py ir_idl.py minorCodes.py

ifdef OPEN_SSL_ROOT
FILES += sslTP.py
endif

export:: $(FILES)
	@(dir="$(PYLIBDIR)"; \
          for file in $^; do \
            $(ExportFileToDir) \
          done; \
          cd $(PYLIBDIR); \
	  $(PYTHON) -c "import compileall; compileall.compile_dir('.')"; \
	 )

ifdef INSTALLTARGET
install:: $(FILES)
	@(dir="$(INSTALLPYLIBDIR)"; \
          for file in $^; do \
            $(ExportFileToDir) \
          done; \
          cd $(INSTALLPYLIBDIR); \
	  $(PYTHON) -c "import compileall; compileall.compile_dir('.')"; \
	 )
endif
