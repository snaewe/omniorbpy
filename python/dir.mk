PYBINDIR = $(EXPORT_TREE)/bin/scripts
PYLIBDIR = $(EXPORT_TREE)/lib/python

SUBDIRS = omniORB CosNaming POA_CosNaming

ifeq ($(PYTHON),)
PYTHON = python
endif

all::
	@$(MakeSubdirs)

export::
	@$(MakeSubdirs)

export:: Naming_idl.py
	@(file="Naming_idl.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))

export::
	@(set -x; \
	cd $(PYLIBDIR); \
	$(PYTHON) -c "import compileall; compileall.compile_dir('.')"; \
	)
