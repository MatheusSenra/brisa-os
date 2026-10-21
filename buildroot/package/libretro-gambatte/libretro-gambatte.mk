################################################################################
#
# libretro-gambatte
#
################################################################################

LIBRETRO_GAMBATTE_VERSION = d9d6cd06382d1ced30de34d56d3609452323dab1
LIBRETRO_GAMBATTE_SITE = $(call github,libretro,gambatte-libretro,$(LIBRETRO_GAMBATTE_VERSION))
LIBRETRO_GAMBATTE_LICENSE = GPL-2.0

define LIBRETRO_GAMBATTE_BUILD_CMDS
	$(call BRISA_LIBRETRO_BUILD,$(@D),-f Makefile.libretro)
endef

define LIBRETRO_GAMBATTE_INSTALL_TARGET_CMDS
	$(call BRISA_LIBRETRO_INSTALL,$(@D)/gambatte_libretro.so)
endef

$(eval $(generic-package))
