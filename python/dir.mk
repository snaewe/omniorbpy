PYBINDIR = $(EXPORT_TREE)/bin/scripts
PYLIBDIR = $(EXPORT_TREE)/lib/python

SUBDIRS = omniORB CosNaming CosNaming__POA

ifeq ($(PYTHON),)
PYTHON = python
endif

# This rather bizarre construction contrives to build the Naming
# service stubs for Windows without requiring oidlwrapper, since it
# isn't present on omniORB 2.8.

ifdef Win32Platform

Naming_idl.py: Naming.idl
	$(CP) $^ .
	$(BASE_OMNI_TREE)/$(BINDIR)/omnicpp -lang-c++ -undef Naming.idl >pnaming.idl
	$(BASE_OMNI_TREE)/$(BINDIR)/omniidl -v -N -bpython pnaming.idl
	$(RM) pnaming.idl
	$(RM) Naming.idl
else
Naming_idl.py: Naming.idl
	$(BASE_OMNI_TREE)/$(BINDIR)/omniidl -v -bpython $^
endif

all::
	@$(MakeSubdirs)

export::
	@$(MakeSubdirs)

export:: CORBA.py
	@(file="CORBA.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))

export:: PortableServer.py
	@(file="PortableServer.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))

export:: PortableServer__POA.py
	@(file="PortableServer__POA.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))

export:: Naming_idl.py
	@(file="Naming_idl.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))

export::
	@(set -x; \
	cd $(PYLIBDIR); \
	$(PYTHON) -c "import compileall; compileall.compile_dir('.')"; \
	)
