PYLIBDIR = $(EXPORT_TREE)/lib/python/CosNaming__POA

export:: __init__.py
	@(file="__init__.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))
