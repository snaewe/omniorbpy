PYBINDIR = $(EXPORT_TREE)/bin/scripts
PYLIBDIR = $(EXPORT_TREE)/lib/python

SUBDIRS = omniORB omniidl

all::
	@$(MakeSubdirs)

export::
	@$(MakeSubdirs)
