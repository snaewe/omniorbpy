PYLIBDIR = $(EXPORT_TREE)/lib/python/omniORB

export:: __init__.py
	@(file="__init__.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))

export:: CORBA.py
	@(file="CORBA.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))

export:: PortableServer.py
	@(file="PortableServer.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))

export:: tcInternal.py
	@(file="tcInternal.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))
