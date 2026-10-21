################################################################################
#
# brisa-retroarch
#
################################################################################

BRISA_RETROARCH_VERSION = v1.22.2
BRISA_RETROARCH_SITE = $(call github,libretro,RetroArch,$(BRISA_RETROARCH_VERSION))
BRISA_RETROARCH_LICENSE = GPL-3.0+
BRISA_RETROARCH_LICENSE_FILES = COPYING
BRISA_RETROARCH_DEPENDENCIES = host-pkgconf alsa-lib zlib libegl libgles udev

# Só o necessário para um portátil sem teclado: Mali fbdev + ALSA + udev.
BRISA_RETROARCH_CONF_OPTS = \
	--disable-x11 --disable-wayland --disable-kms --disable-vulkan \
	--disable-qt --disable-sdl --disable-sdl2 --disable-ffmpeg \
	--disable-pulse --disable-oss --disable-jack --disable-discord \
	--disable-materialui --disable-xmb --disable-freetype \
	--enable-egl --enable-opengles --enable-opengles3 --enable-mali_fbdev \
	--enable-alsa --enable-udev --enable-zlib --enable-threads \
	--enable-networking --enable-rgui --enable-ozone

define BRISA_RETROARCH_CONFIGURE_CMDS
	(cd $(@D) && rm -f config.cache && \
		$(TARGET_CONFIGURE_OPTS) \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		PKG_CONF_PATH="$(PKG_CONFIG_HOST_BINARY)" \
		./configure --prefix=/usr $(BRISA_RETROARCH_CONF_OPTS))
endef

define BRISA_RETROARCH_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D) \
		CC="$(TARGET_CC)" CXX="$(TARGET_CXX)" LD="$(TARGET_CXX)"
endef

# Sem o contexto fbdev_mali o RetroArch abre sem vídeo no aparelho: falha no build.
define BRISA_RETROARCH_INSTALL_TARGET_CMDS
	grep -q fbdev_mali $(@D)/retroarch || \
		{ echo "brisa-retroarch: compilado sem o contexto fbdev_mali"; exit 1; }
	$(INSTALL) -D -m 0755 $(@D)/retroarch $(TARGET_DIR)/usr/bin/retroarch
endef

$(eval $(generic-package))
