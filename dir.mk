SUBDIRS = modules python omniidl_be

ifeq ($(OMNIORB_VERSION),)
SUBDIRS += omniidl
endif

all::
	@$(MakeSubdirs)

export::
	@$(MakeSubdirs)
