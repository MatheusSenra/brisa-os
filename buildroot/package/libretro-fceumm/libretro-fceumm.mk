################################################################################
#
# libretro-fceumm
#
################################################################################

LIBRETRO_FCEUMM_VERSION = ac0d00a7fc01166f84c7bac01d1084ebfd2b865e
LIBRETRO_FCEUMM_SITE = $(call github,libretro,libretro-fceumm,$(LIBRETRO_FCEUMM_VERSION))
LIBRETRO_FCEUMM_LICENSE = GPL-2.0+

define LIBRETRO_FCEUMM_BUILD_CMDS
	$(call BRISA_LIBRETRO_BUILD,$(@D),-f Makefile.libretro)
endef

define LIBRETRO_FCEUMM_INSTALL_TARGET_CMDS
	$(call BRISA_LIBRETRO_INSTALL,$(@D)/fceumm_libretro.so)
endef

$(eval $(generic-package))
