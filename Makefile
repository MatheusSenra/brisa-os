# Brisa OS: comandos principais. Rode dentro do WSL (ou Linux), na raiz do projeto.
#
#   make deps                    instala as dependências (Ubuntu/Debian)
#   make vendor STOCK=<imagem>   extrai do firmware stock o que é reaproveitado
#   make                         compila o sistema e gera a imagem para gravar
#   make config-nogpu rootfs     compila sem Mali/RetroArch (não precisa do stock)
#   make ui-run                  abre a interface no PC
#   make test                    testes que rodam sem o aparelho
#   make check                   confere o rootfs compilado (GPU, bibliotecas, vendor)
#   make clean-stock             apaga as cópias de trabalho do firmware stock
#   make distclean               apaga tudo que o build gerou fora do projeto

BR_VERSION ?= 2026.02.3
WORK       ?= $(HOME)/brisa-work
BR_DIR     ?= $(WORK)/buildroot
OUT        ?= $(WORK)/output
IMAGE      ?= $(WORK)/brisa-rg35xxh.img
IMAGE_OPTS ?= --xz
TOP        := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
# A imagem compactada vai para dentro do projeto, onde o Windows a enxerga.
RELEASE    ?= $(TOP)/out
DEFCONFIG  := brisa_rg35xxh_defconfig

# O WSL coloca no PATH as pastas do Windows (com espaços) e roda como root;
# o Buildroot recusa os dois sem estes ajustes.
export PATH := /usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
export FORCE_UNSAFE_CONFIGURE := 1

BR_MAKE = $(MAKE) -C $(BR_DIR) O=$(OUT) BR2_EXTERNAL=$(TOP)/buildroot

all: image

deps:
	sudo apt-get install -y build-essential git fdisk mtools dosfstools fakeroot pkg-config \
		bc unzip p7zip-full xz-utils file cpio rsync wget e2fsprogs libncurses-dev \
		libsdl2-dev fonts-dejavu-core python3-pil

$(BR_DIR):
	git clone --depth 1 -b $(BR_VERSION) https://gitlab.com/buildroot.org/buildroot.git $@

vendor:
	@test -n "$(STOCK)" || { echo "uso: make vendor STOCK=/caminho/firmware-stock.img"; exit 1; }
	scripts/extract-vendor.sh "$(STOCK)"

config: | $(BR_DIR)
	$(BR_MAKE) $(DEFCONFIG)

# Sem o blob Mali não há RetroArch, mas dá para validar todo o resto.
config-nogpu: | $(BR_DIR)
	$(BR_MAKE) $(DEFCONFIG)
	cat buildroot/board/rg35xxh/nogpu.fragment >> $(OUT)/.config
	$(BR_MAKE) olddefconfig

$(OUT)/.config:
	$(MAKE) config

rootfs: $(OUT)/.config
	$(BR_MAKE)

# Com --xz, só o .img.xz fica (em out/); a imagem crua de ~15 GB é apagada.
# O balenaEtcher e o Rufus gravam o .img.xz direto.
image: rootfs check
	scripts/build-image.sh $(OUT)/images/rootfs.tar $(IMAGE) $(IMAGE_OPTS)
	@if [ -f $(IMAGE).xz ]; then \
		mkdir -p $(RELEASE) && mv -f $(IMAGE).xz $(RELEASE)/ && rm -f $(IMAGE) && \
		echo "==> imagem: $(RELEASE)/$(notdir $(IMAGE)).xz"; \
	fi

check:
	scripts/check-rootfs.sh $(OUT)

menuconfig: $(OUT)/.config
	$(BR_MAKE) menuconfig

# Recompila só a interface (depois de mexer em ui/) e refaz o rootfs.
rebuild-ui:
	$(BR_MAKE) brisa-ui-rebuild all

ui:
	$(MAKE) -C ui

ui-run:
	$(MAKE) -C ui run

ui-test:
	$(MAKE) -C ui test

test: ui-test
	scripts/test/image-roundtrip.sh

clean:
	rm -rf $(OUT)

# O que o projeto precisa do stock já está em vendor/; o cache em $(WORK)/stock
# (partições copiadas e árvore extraída) só acelera um novo 'make vendor'.
clean-stock:
	rm -rf $(WORK)/stock $(WORK)/img.*

# Deixa só o clone do Buildroot e os downloads (dl/), para não baixar tudo de novo.
distclean: clean clean-stock
	rm -f $(IMAGE) $(IMAGE).xz

.PHONY: all deps vendor config config-nogpu rootfs image check menuconfig rebuild-ui ui ui-run ui-test test clean clean-stock distclean
