################################################################################
#
# trivmod
#
################################################################################

TRIVMOD_VERSION = v1.0
TRIVMOD_SITE = https://github.com/dororiya/trivmod-package.git
TRIVMOD_SITE_METHOD = git
TRIVMOD_LICENSE = GPL-2.0

$(eval $(kernel-module))
$(eval $(generic-package))
