PYLIBDIR = $(EXPORT_TREE)/lib/python/omniORB


ifdef Win32Platform

ir_idl.py: ir.idl
	$(CP) $^ .
	$(CP) $(BASE_OMNI_TREE)/idl/corbaidl.idl .
	$(BASE_OMNI_TREE)/$(BINDIR)/omnicpp -I. -lang-c++ -undef ir.idl >pir.idl
	$(BASE_OMNI_TREE)/$(BINDIR)/omniidl -v -N -bpython -Wbinline -Wbno_package pir.idl
	$(RM) pir.idl
	$(RM) ir.idl
	$(RM) corbaidl.idl
else
ir_idl.py: ir.idl
	$(BASE_OMNI_TREE)/$(BINDIR)/omniidl -v -I$(BASE_OMNI_TREE)/idl -bpython -Wbinline -Wbno_package $^
endif


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

export:: ir_idl.py
	@(file="ir_idl.py"; dir="$(PYLIBDIR)"; $(ExportFileToDir))
