################################################################################
#
# brisa-ui
#
################################################################################

BRISA_UI_VERSION = 1.0
BRISA_UI_SITE = $(BR2_EXTERNAL_BRISA_PATH)/../ui
BRISA_UI_SITE_METHOD = local
BRISA_UI_LICENSE = MIT

# A cópia de ui/ pode trazer o binário de PC (make ui-run); limpa antes.
define BRISA_UI_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D) clean
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D) PLATFORM=fbdev \
		CC="$(TARGET_CC)" CFLAGS="$(TARGET_CFLAGS)" LDFLAGS="$(TARGET_LDFLAGS)"
endef

define BRISA_UI_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/brisa-ui $(TARGET_DIR)/usr/bin/brisa-ui
endef

$(eval $(generic-package))
