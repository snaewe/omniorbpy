PYLIBDIR = $(EXPORT_TREE)/lib/python/omniidl_be

ifeq ($(PYTHON),)
PYTHON = python
endif

export:: __init__.py
	@(file="__init__.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))

export:: python.py
	@(file="python.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))

export::
	@(set -x; \
	cd $(PYLIBDIR); \
	$(PYTHON) -c "import compileall; compileall.compile_dir('.')"; \
	)
