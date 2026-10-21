################################################################################
#
# libretro-pcsx-rearmed
#
################################################################################

LIBRETRO_PCSX_REARMED_VERSION = c8816799b50388e61cfe237fe2cdbb7d8175f20a
LIBRETRO_PCSX_REARMED_SITE = https://github.com/libretro/pcsx_rearmed.git
LIBRETRO_PCSX_REARMED_SITE_METHOD = git
LIBRETRO_PCSX_REARMED_GIT_SUBMODULES = YES
LIBRETRO_PCSX_REARMED_LICENSE = GPL-2.0

define LIBRETRO_PCSX_REARMED_BUILD_CMDS
	$(call BRISA_LIBRETRO_BUILD,$(@D),-f Makefile.libretro ARCH=arm64 DYNAREC=ari64)
endef

define LIBRETRO_PCSX_REARMED_INSTALL_TARGET_CMDS
	$(call BRISA_LIBRETRO_INSTALL,$(@D)/pcsx_rearmed_libretro.so)
endef

$(eval $(generic-package))
