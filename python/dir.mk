PYLIBROOT= $(EXPORT_TREE)/lib/python
PYLIBDIR = $(PYLIBROOT)
INSTALLPYLIBDIR = $(INSTALLPYTHONDIR)

SUBDIRS = omniORB CosNaming CosNaming__POA

ifeq ($(PYTHON),)
PYTHON = python
endif


Naming_idl.py: Naming.idl
	$(OMNIIDL) -v -p$(BASE_OMNI_TREE)/omniidl_be -bpython $^

all:: Naming_idl.py

all::
	@$(MakeSubdirs)

export::
	@$(MakeSubdirs)

ifdef INSTALLTARGET
install::
	@$(MakeSubdirs)
endif

FILES = CORBA.py PortableServer.py PortableServer__POA.py Naming_idl.py

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
