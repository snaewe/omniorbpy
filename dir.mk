ifeq ($(OMNIORB_VERSION),)
SUBDIRS = omniidl
endif

SUBDIRS += modules omniidl_be python


pydistdate.hh: $(BASE_OMNI_TREE)/src/lib/omniORBpy/update.log
	$(PYTHON) $(BASE_OMNI_TREE)/bin/scripts/distdate.py OMNIORBPY <$(BASE_OMNI_TREE)/src/lib/omniORBpy/update.log >pydistdate.hh

all:: pydistdate.hh

export:: pydistdate.hh
	@(file=$^; dir="$(EXPORT_TREE)/$(INCDIR)/omniORB4"; \
	 $(ExportFileToDir))

all::
	@$(MakeSubdirs)

export::
	@$(MakeSubdirs)
