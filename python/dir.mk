PYBINDIR = $(EXPORT_TREE)/bin/scripts
PYLIBDIR = $(EXPORT_TREE)/lib/python

SUBDIRS = omniORB omnistub CosNaming POA_CosNaming

all::
	@$(MakeSubdirs)

export::
	@$(MakeSubdirs)
