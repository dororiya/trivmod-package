TRIVMOD_VERSION = v1.2
TRIVMOD_SITE = $(call github,dororiya,trivmod-package,$(TRIVMOD_VERSION))
TRIVMOD_LICENSE = GPL-2.0
TRIVMOD_MODULE_SUBDIRS = cdev

$(eval $(kernel-module))
$(eval $(generic-package))
