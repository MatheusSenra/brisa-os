################################################################################
#
# libretro-mgba
#
################################################################################

LIBRETRO_MGBA_VERSION = 7a12d6d4b9acb14c0ae62c9166b6a2f3d08007f6
LIBRETRO_MGBA_SITE = $(call github,libretro,mgba,$(LIBRETRO_MGBA_VERSION))
LIBRETRO_MGBA_LICENSE = MPL-2.0

# O repositório segue o mGBA original: o core sai do CMake com BUILD_LIBRETRO.
# Só o core, sem frontends nem dependências opcionais.
LIBRETRO_MGBA_CONF_OPTS = \
	-DBUILD_LIBRETRO=ON -DBUILD_QT=OFF -DBUILD_SDL=OFF -DBUILD_GL=OFF -DBUILD_GLES2=OFF \
	-DBUILD_GLES3=OFF -DBUILD_SHARED=OFF -DBUILD_STATIC=OFF -DSKIP_LIBRARY=ON \
	-DENABLE_SCRIPTING=OFF -DENABLE_DEBUGGERS=OFF -DENABLE_GDB_STUB=OFF \
	-DUSE_FFMPEG=OFF -DUSE_ZLIB=OFF -DUSE_PNG=OFF -DUSE_LIBZIP=OFF -DUSE_MINIZIP=OFF \
	-DUSE_SQLITE3=OFF -DUSE_ELF=OFF -DUSE_LZMA=OFF -DUSE_EDITLINE=OFF -DUSE_EPOXY=OFF \
	-DUSE_DISCORD_RPC=OFF -DUSE_LUA=OFF -DUSE_JSON_C=OFF

define LIBRETRO_MGBA_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $$(find $(@D) -name mgba_libretro.so | head -n1) \
		$(TARGET_DIR)/usr/lib/libretro/mgba_libretro.so
endef

$(eval $(cmake-package))
