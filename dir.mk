SUBDIRS = modules omniidl_be python

all::
	@$(MakeSubdirs)

export::
	@$(MakeSubdirs)

ifdef INSTALLTARGET
SUBDIRS += include
install::
	@$(MakeSubdirs)
endif
