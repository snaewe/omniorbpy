PYLIBDIR = $(EXPORT_TREE)/lib/python/POA_CosNaming

export:: __init__.py
	@(file="__init__.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))
