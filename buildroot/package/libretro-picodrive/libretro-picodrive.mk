################################################################################
#
# libretro-picodrive
#
################################################################################

LIBRETRO_PICODRIVE_VERSION = 1890c2932234c9d30f4cd3851d02228baae8f09e
LIBRETRO_PICODRIVE_SITE = https://github.com/libretro/picodrive.git
LIBRETRO_PICODRIVE_SITE_METHOD = git
LIBRETRO_PICODRIVE_GIT_SUBMODULES = YES
LIBRETRO_PICODRIVE_LICENSE = PicoDrive (uso não comercial)

define LIBRETRO_PICODRIVE_BUILD_CMDS
	$(call BRISA_LIBRETRO_BUILD,$(@D),-f Makefile.libretro)
endef

define LIBRETRO_PICODRIVE_INSTALL_TARGET_CMDS
	$(call BRISA_LIBRETRO_INSTALL,$(@D)/picodrive_libretro.so)
endef

$(eval $(generic-package))
