ifeq ($(OMNIORB_VERSION),)
SUBDIRS = omniidl
endif

SUBDIRS += modules omniidl_be python

all::
	@$(MakeSubdirs)

export::
	@$(MakeSubdirs)
