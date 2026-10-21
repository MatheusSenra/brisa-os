# Compila um core libretro com o toolchain do Buildroot.
# CFLAGS vão pelo ambiente para que o "CFLAGS +=" dos Makefiles dos cores
# continue valendo; CC/CXX/AR vão na linha de comando para vencer "CC = gcc".
#   $(1) = diretório do código
#   $(2) = argumentos extras do make (Makefile, ARCH, DYNAREC...)
# Sem as macros de arquivo grande: em aarch64 o off_t já tem 64 bits, e elas
# fazem o código chamar fopen64 por fora da camada VFS do libretro.
BRISA_LIBRETRO_NOLFS = -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

define BRISA_LIBRETRO_BUILD
	$(TARGET_MAKE_ENV) \
		CFLAGS="$(filter-out $(BRISA_LIBRETRO_NOLFS),$(TARGET_CFLAGS))" \
		CXXFLAGS="$(filter-out $(BRISA_LIBRETRO_NOLFS),$(TARGET_CXXFLAGS))" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		$(MAKE) -C $(1) platform=unix \
			CC="$(TARGET_CC)" CXX="$(TARGET_CXX)" AR="$(TARGET_AR)" \
			$(2)
endef

# Instala um core em /usr/lib/libretro.
#   $(1) = caminho do .so gerado
define BRISA_LIBRETRO_INSTALL
	$(INSTALL) -D -m 0755 $(1) $(TARGET_DIR)/usr/lib/libretro/$(notdir $(1))
endef

include $(sort $(wildcard $(BR2_EXTERNAL_BRISA_PATH)/package/*/*.mk))
