<p align="center">
  <img src="docs/img/banner.svg" alt="Brisa OS" width="100%">
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/licen%C3%A7a-MIT-4fd1c5?style=flat-square" alt="Licença MIT"></a>
  <img src="https://img.shields.io/badge/aparelho-RG35XX%20H-151a26?style=flat-square" alt="RG35XX H">
  <img src="https://img.shields.io/badge/SoC-Allwinner%20H700-151a26?style=flat-square" alt="Allwinner H700">
  <img src="https://img.shields.io/badge/Buildroot-2026.02%20LTS-151a26?style=flat-square" alt="Buildroot 2026.02 LTS">
  <img src="https://img.shields.io/badge/status-alpha-f2b34c?style=flat-square" alt="Status: alpha">
</p>

<p align="center">
  <b>Liga rápido, fica fora do caminho e deixa o jogo começar.</b><br>
  Um firmware enxuto, com interface própria, para o portátil Anbernic RG35XX H.
</p>

<p align="center">
  <a href="#-começando">Começando</a> ·
  <a href="#-como-funciona">Como funciona</a> ·
  <a href="#-uso">Uso</a> ·
  <a href="#-desenvolvimento">Desenvolvimento</a> ·
  <a href="#-roadmap">Roadmap</a>
</p>

<p align="center">
  <img src="docs/img/telas.png" alt="Telas do Brisa OS: sistemas, lista de jogos e ajustes" width="100%">
</p>

## ✨ Destaques

| | |
|---|---|
| ⚡ **Boot rápido** | BusyBox init e um `rcS` mínimo: monta o cartão e abre a interface. Rede, udev e áudio sobem em segundo plano. |
| 🎨 **Interface própria** | Escrita em C, desenhada direto no framebuffer, sem depender da GPU. Navegação com repetição de teclas, recentes, ajustes e teste de controles. |
| 🎮 **Emulação ajustada** | RetroArch para Mali-G31 (fbdev), cores escolhidos para o H700, governor da CPU por sistema, run-ahead nos sistemas leves e "continuar de onde parou". |
| 📶 **Rede sem complicação** | Wi-Fi configurado por um arquivo no cartão, SSH e SFTP com um toque, relógio acertado via NTP. |
| 🪶 **Leve** | Rootfs de ~52 MB. A interface depende só da libc. |
| 🛟 **Sem risco** | O aparelho inicia pelo microSD. Não gostou? Troque o cartão. |

## 🚦 Status

O Brisa está em **alpha**. Já funciona e foi testado:

- ✅ Build completo do sistema (toolchain, rootfs, interface e os 6 cores)
- ✅ Interface no PC, com testes automáticos que geram capturas de tela
- ✅ Extração do firmware stock e montagem da imagem, testadas de ponta a ponta

Falta validar no aparelho:

- ⏳ RetroArch com o blob Mali da Anbernic (precisa da imagem stock para compilar)
- ⏳ Primeiro boot no RG35XX H real

## 🚀 Começando

### Requisitos

- Linux ou **WSL2** (Ubuntu 22.04 ou mais novo), com ~15 GB livres
- A **imagem do firmware stock** do RG35XX H (site da Anbernic, ou uma cópia do seu cartão original)
- Um cartão microSD

### Compilar

```bash
git clone https://github.com/MatheusSenra/brisa-os.git
cd brisa-os
make deps
```

Extraia do firmware stock o que o Brisa reaproveita (kernel, módulos, driver da GPU):

```bash
make vendor STOCK=/caminho/para/RG35XXH-stock.img.7z
```

Confira o relatório, depois compile. A primeira compilação leva cerca de uma hora.

```bash
cat vendor/info/report.txt
```

```bash
make
```

O resultado fica em `~/brisa-work/brisa-rg35xxh.img` (e `.img.xz`).

### Gravar

Grave a imagem no cartão com o [balenaEtcher](https://etcher.balena.io/) ou o [Rufus](https://rufus.ie/), coloque no slot **TF1** e ligue.

> [!TIP]
> Use `make IMAGE_OPTS="--xz --wipe-storage"` para também formatar a partição de dados do cartão como FAT32.

## 🧭 Como funciona

O Brisa **mantém o bootloader e o kernel 4.9 da Anbernic** e troca só o sistema de arquivos, o mesmo caminho do muOS. Tela, áudio, botões e Wi-Fi funcionam com os drivers originais, e o resto é nosso.

```mermaid
flowchart LR
    stock[("Imagem stock<br/>da Anbernic")]
    vendor["vendor/<br/>módulos, firmware Wi-Fi,<br/>libmali, perfis de controle"]
    br["Buildroot<br/>glibc · BusyBox · brisa-ui<br/>RetroArch · cores · rede"]
    rootfs[("rootfs.tar")]
    img[("brisa-rg35xxh.img")]

    stock -- "extract-vendor.sh" --> vendor
    vendor --> br
    br --> rootfs
    stock -- "boot, kernel e partições" --> build
    rootfs --> build["build-image.sh<br/>troca só o rootfs"]
    build --> img
```

**Na inicialização**, o `rcS` monta o cartão (o TF2 tem prioridade para as ROMs, se existir) e grava os caminhos em `/run/brisa/env`. Em seguida o init abre a `brisa-ui`. Ao escolher um jogo, a interface solta a tela e chama o `brisa-launch`, que ajusta a CPU e abre o RetroArch com a configuração do sistema. Enquanto o jogo roda, a interface continua ouvindo os atalhos globais: volume, brilho e POWER.

## 🎮 Uso

### Sistemas

| Sistema | Pasta em `Roms/` | Core | Extensões |
|---|---|---|---|
| Game Boy | `GB` | Gambatte | `.gb` `.zip` |
| Game Boy Color | `GBC` | Gambatte | `.gbc` `.gb` `.zip` |
| Game Boy Advance | `GBA` | mGBA | `.gba` `.zip` |
| Nintendo (NES) | `NES`, `FC` | FCEUmm | `.nes` `.fds` `.unf` `.zip` |
| Super Nintendo | `SNES`, `SFC` | Snes9x | `.sfc` `.smc` `.zip` |
| Master System | `MS`, `SMS` | PicoDrive | `.sms` `.zip` |
| Game Gear | `GG` | PicoDrive | `.gg` `.zip` |
| Mega Drive | `MD`, `Genesis` | PicoDrive | `.md` `.gen` `.smd` `.bin` `.zip` |
| Mega CD | `SEGACD`, `MegaCD` | PicoDrive | `.cue` `.chd` |
| Sega 32X | `32X` | PicoDrive | `.32x` `.zip` |
| PlayStation | `PS`, `PS1`, `PSX` | PCSX ReARMed | `.cue` `.chd` `.pbp` `.m3u` |

Para adicionar um sistema, basta uma linha em [`systems.conf`](buildroot/board/rg35xxh/rootfs-overlay/etc/brisa/systems.conf).

### Cartão

No primeiro boot, o Brisa cria a estrutura de pastas:

```text
Roms/GB  Roms/GBA  Roms/SNES  Roms/PS  ...   ← seus jogos
Brisa/bios                                   ← BIOS (ex.: scph5501.bin para PS1)
Brisa/saves   Brisa/states                   ← saves e estados salvos
Brisa/wifi.conf.exemplo                      ← copie como wifi.conf
```

### Botões

| Botão | Ação |
|---|---|
| **START** (na lista) | Ajustes |
| **SELECT** (na lista) | Atualiza a lista de jogos |
| **MENU** + ↑/↓ | Brilho (funciona também no jogo) |
| **START** + **SELECT** (no jogo) | Menu do RetroArch |
| **POWER** | Menu de energia; no jogo, sai salvando |
| **Volume** | Funciona na interface e no jogo |

### Rede

Crie `Brisa/wifi.conf` no cartão:

```ini
REDE=Nome da minha rede
SENHA=minha senha
```

Depois ligue o Wi-Fi em **Ajustes**. Com **SSH e SFTP** ligado, conecte como `root` (senha `brisa`) e troque a senha com `passwd`.

## 🛠 Desenvolvimento

### Estrutura

```text
brisa-os/
├── ui/                        interface (C, framebuffer + evdev; SDL2 no PC)
├── buildroot/                 árvore externa do Buildroot
│   ├── configs/               brisa_rg35xxh_defconfig
│   ├── package/               brisa-ui, brisa-mali, brisa-retroarch, cores libretro...
│   └── board/rg35xxh/         rootfs-overlay (rcS, scripts brisa-*, configs)
├── scripts/                   extração do stock, montagem da imagem, testes
└── docs/                      imagens deste README
```

### Interface no PC

```bash
make ui-run
```

| Tecla | Botão | Tecla | Botão |
|---|---|---|---|
| Setas | Direcional | Enter | START |
| X | A | Shift direito | SELECT |
| Z | B | Esc | MENU |
| Q / W | L1 / R1 | P | POWER |

### Testes

```bash
make test
```

O comando roda a interface sem janela, seguindo um roteiro que salva capturas em `ui/build/shots/`. Depois executa a extração e a montagem de imagem com um firmware stock falso.

### Sem o firmware stock

Dá para compilar e validar todo o sistema, exceto o RetroArch:

```bash
make config-nogpu rootfs
```

### Atualizar só a interface no aparelho

```bash
make rebuild-ui
```

```bash
scp ~/brisa-work/output/target/usr/bin/brisa-ui root@<ip-do-aparelho>:/usr/bin/
```

## 🗺 Roadmap

- [x] Interface com sistemas, jogos, recentes e ajustes
- [x] Wi-Fi, SSH e SFTP
- [x] Governor da CPU por sistema, run-ahead e retomada automática
- [ ] Primeiro boot validado no RG35XX H
- [ ] Teclado na tela para configurar o Wi-Fi
- [ ] Suspender e retomar pelo botão POWER
- [ ] Capas dos jogos e favoritos
- [ ] Mais sistemas: N64, PSP e Dreamcast, com ajustes por jogo
- [ ] Suporte a outros aparelhos H700 (RG35XX Plus, SP, 2024, RG40XX)

## 🤝 Contribuindo

Issues e pull requests são bem-vindos. Antes de abrir um PR, rode `make test`.
Se você tem um RG35XX H, o mais útil agora é testar no aparelho e contar o que aconteceu: tela, botões, áudio e Wi-Fi.

## 📄 Licença e avisos

O código deste repositório está sob a licença [MIT](LICENSE).
O RetroArch e os cores são baixados durante a compilação e mantêm suas próprias licenças (GPL e outras).

O Brisa OS **não** é afiliado à Anbernic. Este repositório não contém nem distribui ROMs, BIOS ou arquivos proprietários do firmware stock: eles são extraídos localmente, da sua própria cópia, pelo `make vendor`.
