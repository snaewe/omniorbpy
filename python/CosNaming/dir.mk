PYLIBDIR = $(EXPORT_TREE)/lib/python/CosNaming

export:: __init__.py
	@(file="__init__.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))
