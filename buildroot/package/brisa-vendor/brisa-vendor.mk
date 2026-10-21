################################################################################
#
# brisa-vendor
#
################################################################################

# Sem código-fonte: copia o diretório gerado por scripts/extract-vendor.sh.
BRISA_VENDOR_VERSION = stock
BRISA_VENDOR_SOURCE =
BRISA_VENDOR_LICENSE = PROPRIETARY
BRISA_VENDOR_DIR = $(call qstrip,$(BR2_PACKAGE_BRISA_VENDOR_DIR))

define BRISA_VENDOR_INSTALL_TARGET_CMDS
	if [ ! -f $(BRISA_VENDOR_DIR)/hw.conf ]; then \
		echo "brisa-vendor: $(BRISA_VENDOR_DIR) vazio; o sistema vai sem módulos do stock."; \
		echo "              Rode 'make vendor STOCK=<imagem>' para extrair do firmware stock."; \
	fi
	if [ -d $(BRISA_VENDOR_DIR)/modules ]; then \
		mkdir -p $(TARGET_DIR)/lib/modules && \
		cp -a $(BRISA_VENDOR_DIR)/modules/. $(TARGET_DIR)/lib/modules/; \
	fi
	if [ -d $(BRISA_VENDOR_DIR)/firmware ]; then \
		mkdir -p $(TARGET_DIR)/lib/firmware && \
		cp -a $(BRISA_VENDOR_DIR)/firmware/. $(TARGET_DIR)/lib/firmware/; \
	fi
	if [ -d $(BRISA_VENDOR_DIR)/autoconfig ]; then \
		mkdir -p $(TARGET_DIR)/usr/share/brisa/autoconfig && \
		cp -a $(BRISA_VENDOR_DIR)/autoconfig/. $(TARGET_DIR)/usr/share/brisa/autoconfig/; \
	fi
	if [ -f $(BRISA_VENDOR_DIR)/retroarch/input-stock.cfg ]; then \
		$(INSTALL) -D -m 0644 $(BRISA_VENDOR_DIR)/retroarch/input-stock.cfg \
			$(TARGET_DIR)/etc/brisa/retroarch/input-stock.cfg; \
	fi
	if [ -f $(BRISA_VENDOR_DIR)/hw.conf ]; then \
		$(INSTALL) -D -m 0644 $(BRISA_VENDOR_DIR)/hw.conf $(TARGET_DIR)/etc/brisa/hw.conf; \
	fi
endef

$(eval $(generic-package))
