################################################################################
#
# brisa-mali
#
################################################################################

BRISA_MALI_VERSION = stock
BRISA_MALI_SOURCE =
BRISA_MALI_LICENSE = PROPRIETARY
BRISA_MALI_INSTALL_STAGING = YES
BRISA_MALI_PROVIDES = libegl libgles
BRISA_MALI_DEPENDENCIES = mesa3d-headers
BRISA_MALI_DIR = $(call qstrip,$(BR2_PACKAGE_BRISA_VENDOR_DIR))/mali

# Copia o blob e cria os nomes que o RetroArch procura (libEGL, libGLESv2),
# apontando para a libmali quando o stock não traz esses links.
#   $(1) = raiz de destino (staging ou target)
define BRISA_MALI_INSTALL_LIBS
	if ! ls $(BRISA_MALI_DIR)/libmali* $(BRISA_MALI_DIR)/libMali* >/dev/null 2>&1; then \
		echo "brisa-mali: blob Mali não encontrado em $(BRISA_MALI_DIR)."; \
		echo "            Rode 'make vendor STOCK=<imagem>' ou use 'make config-nogpu'."; \
		exit 1; \
	fi
	mkdir -p $(1)/usr/lib
	cp -a $(BRISA_MALI_DIR)/*.so* $(1)/usr/lib/
	cd $(1)/usr/lib && \
		blob=$$(ls libmali.so* libMali.so* 2>/dev/null | head -n1) && \
		for l in libEGL.so libEGL.so.1 libGLESv2.so libGLESv2.so.2 libGLESv1_CM.so libGLESv1_CM.so.1; do \
			[ -e $$l ] || ln -sf $$blob $$l; \
		done
endef

define BRISA_MALI_INSTALL_STAGING_CMDS
	$(call BRISA_MALI_INSTALL_LIBS,$(STAGING_DIR))
	mkdir -p $(STAGING_DIR)/usr/lib/pkgconfig
	printf 'prefix=/usr\nlibdir=$${prefix}/lib\nincludedir=$${prefix}/include\n\nName: egl\nDescription: Mali EGL\nVersion: 10.0\nLibs: -L$${libdir} -lEGL\nCflags: -I$${includedir}\n' \
		> $(STAGING_DIR)/usr/lib/pkgconfig/egl.pc
	printf 'prefix=/usr\nlibdir=$${prefix}/lib\nincludedir=$${prefix}/include\n\nName: glesv2\nDescription: Mali OpenGL ES 2/3\nVersion: 10.0\nLibs: -L$${libdir} -lGLESv2\nCflags: -I$${includedir}\n' \
		> $(STAGING_DIR)/usr/lib/pkgconfig/glesv2.pc
endef

define BRISA_MALI_INSTALL_TARGET_CMDS
	$(call BRISA_MALI_INSTALL_LIBS,$(TARGET_DIR))
endef

$(eval $(generic-package))
