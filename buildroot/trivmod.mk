TRIVMOD_VERSION = v1.3
TRIVMOD_SITE = $(call github,dororiya,trivmod-package,$(TRIVMOD_VERSION))
TRIVMOD_LICENSE = GPL-2.0
TRIVMOD_MODULE_SUBDIRS = devfile

$(eval $(kernel-module))
$(eval $(generic-package))
