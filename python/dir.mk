PYBINDIR = $(EXPORT_TREE)/bin/scripts
PYLIBDIR = $(EXPORT_TREE)/lib/python

SUBDIRS = omniORB

all::
	@$(MakeSubdirs)

export::
	@$(MakeSubdirs)
