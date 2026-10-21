#!/bin/sh
# Cria um cartão de mentira com ROMs vazias para testar a interface no PC.
set -e
D="$1"
mkdir -p "$D/Brisa" "$D/vazio"
cd "$D"
mk() { mkdir -p "Roms/$1"; shift_dir="$1"; shift; for f in "$@"; do : > "Roms/$shift_dir/$f"; done; }
mk GB "Tetris (World).gb" "Kirby's Dream Land (USA, Europe).gb" "Pokémon Edição Vermelha (Brasil).gb"
mk GBC "Wario Land 3 (World).gbc" "Shantae (USA).gbc"
mk GBA "Advance Wars (USA).gba" "Castlevania - Aria of Sorrow (USA).gba" "Golden Sun (USA, Europe).gba" \
    "Mother 3 (Japan) [T-En by Tomato v1.3].gba" "Metroid Fusion (USA).gba" "Fire Emblem (USA, Australia).gba"
mk SNES "Chrono Trigger (USA).sfc" "Super Metroid (Japan, USA) (En,Ja).sfc" "Donkey Kong Country (USA) (Rev 2).sfc" \
    "Top Gear (USA).sfc" "Super Mario World (USA).sfc" "EarthBound (USA).sfc" "Secret of Mana (USA).sfc" \
    "Street Fighter II Turbo (USA).sfc" "F-Zero (USA).sfc" "Mega Man X (USA).sfc" "ActRaiser (USA).sfc" \
    "The Legend of Zelda - A Link to the Past (USA).sfc" "Yoshi's Island (USA).sfc"
mk MD "Sonic the Hedgehog 2 (World).md" "Streets of Rage 2 (USA).md" "Mônica no Castelo do Dragão (Brazil).md"
mk PS "Castlevania - Symphony of the Night (USA).chd" "Crash Bandicoot (USA).chd" "Final Fantasy VII (USA).m3u"
mk NES ".escondido.nes" "._lixo-do-mac.nes"
