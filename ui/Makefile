# brisa-ui
#   make                    versão de PC (SDL2), para desenvolver
#   make PLATFORM=fbdev     versão do aparelho (o Buildroot usa esta)
#   make run                abre no PC com ROMs de exemplo
#   make test               roteiro automático sem janela; salva capturas em build/shots

PLATFORM ?= sdl
CC ?= cc
CFLAGS ?= -O2 -g

override CFLAGS += -std=gnu11 -Wall -Wextra -Ithird_party
LDLIBS = -lm

SRC = src/main.c src/common.c src/gfx.c src/font.c src/systems.c src/settings.c \
      src/sysinfo.c src/platform_$(PLATFORM).c
HDR = $(wildcard src/*.h)

ifeq ($(PLATFORM),sdl)
override CFLAGS += $(shell pkg-config --cflags sdl2)
LDLIBS += $(shell pkg-config --libs sdl2)
endif

brisa-ui: $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(LDLIBS)

DEMO = build/demo
ETC = ../buildroot/board/rg35xxh/rootfs-overlay/etc/brisa
# make run ROMS=/caminho/das/roms para ver a interface com os seus jogos
ROMS ?= $(DEMO)/Roms
DEV_ENV = BRISA_ETC=$(ETC) BRISA_DATA=$(DEMO)/Brisa BRISA_ROMS=$(ROMS) BRISA_BIN=$(CURDIR)/pc-bin

$(DEMO):
	tools/mkdemo.sh $(DEMO)

run: brisa-ui $(DEMO)
	$(DEV_ENV) BRISA_SCALE=$(or $(SCALE),1) ./brisa-ui

SHOTS = build/shots
test: brisa-ui $(DEMO)
	rm -rf $(SHOTS) $(DEMO)/Brisa/recent.txt && mkdir -p $(SHOTS)
	$(DEV_ENV) SDL_VIDEODRIVER=dummy BRISA_SCRIPT="SHOT:$(SHOTS)/1-sistemas.bmp,DOWN,DOWN,A,DOWN,DOWN,DOWN,SHOT:$(SHOTS)/2-jogos.bmp,A,B,UP,UP,UP,SHOT:$(SHOTS)/3-recentes.bmp,START,DOWN,DOWN,DOWN,SHOT:$(SHOTS)/4-ajustes.bmp,POWER,SHOT:$(SHOTS)/5-energia.bmp,B,B,QUIT" ./brisa-ui
	BRISA_ETC=$(ETC) BRISA_DATA=$(DEMO)/vazio BRISA_ROMS=$(DEMO)/vazio BRISA_BIN=$(CURDIR)/pc-bin \
		SDL_VIDEODRIVER=dummy BRISA_SCRIPT="SHOT:$(SHOTS)/6-vazio.bmp,QUIT" ./brisa-ui
	python3 tools/sheet.py $(SHOTS)
	@ls $(SHOTS)

clean:
	rm -rf brisa-ui build

.PHONY: run test clean
