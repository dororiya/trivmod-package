TRIVMOD_VERSION = v1.1
TRIVMOD_SITE = $(call github,dororiya,trivmod-package,$(TRIVMOD_VERSION))
TRIVMOD_LICENSE = GPL-2.0

$(eval $(kernel-module))
$(eval $(generic-package))
