PYLIBDIR = $(EXPORT_TREE)/lib/python/omniORB

export:: __init__.py
	@(file="__init__.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))

export:: CORBA.py
	@(file="CORBA.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))

export:: PortableServer.py
	@(file="PortableServer.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))

export:: PortableServer__POA.py
	@(file="PortableServer__POA.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))

export:: tcInternal.py
	@(file="tcInternal.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))

export:: URI.py
	@(file="URI.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))
