PYBINDIR = $(EXPORT_TREE)/bin/scripts
PYLIBDIR = $(EXPORT_TREE)/lib/python

SUBDIRS = omniORB omniidlpy

all::
	@$(MakeSubdirs)

export::
	@$(MakeSubdirs)
