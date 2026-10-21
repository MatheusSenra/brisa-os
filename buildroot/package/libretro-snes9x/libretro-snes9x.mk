################################################################################
#
# libretro-snes9x
#
################################################################################

LIBRETRO_SNES9X_VERSION = fae2fea08f74180759ef540ee94259213f503480
LIBRETRO_SNES9X_SITE = $(call github,libretro,snes9x,$(LIBRETRO_SNES9X_VERSION))
LIBRETRO_SNES9X_LICENSE = Snes9x (uso não comercial)

define LIBRETRO_SNES9X_BUILD_CMDS
	$(call BRISA_LIBRETRO_BUILD,$(@D)/libretro,)
endef

define LIBRETRO_SNES9X_INSTALL_TARGET_CMDS
	$(call BRISA_LIBRETRO_INSTALL,$(@D)/libretro/snes9x_libretro.so)
endef

$(eval $(generic-package))
