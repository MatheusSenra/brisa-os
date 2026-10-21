#!/usr/bin/env bash
# check-rootfs.sh [diretório-output-do-buildroot]
#
# Confere o rootfs compilado antes de gerar a imagem:
#   - RetroArch com o contexto de vídeo fbdev_mali
#   - toda biblioteca que os binários pedem existe no rootfs
#   - arquivos do vendor (módulos, GPU, Wi-Fi, controles) no lugar
set -uo pipefail
O=${1:-$HOME/brisa-work/output}
T=$O/target
RE=$O/host/bin/aarch64-buildroot-linux-gnu-readelf
fail=0
ok() { echo "ok     $*"; }
bad() { echo "FALHA  $*"; fail=1; }
check() { if [ -e "$T/$1" ]; then ok "$1"; else bad "falta $1"; fi; }

[ -d "$T" ] || { echo "rootfs não encontrado em $T (rode 'make rootfs')" >&2; exit 1; }

echo "== RetroArch"
if [ -e "$T/usr/bin/retroarch" ]; then
	if strings "$T/usr/bin/retroarch" | grep fbdev_mali > /dev/null; then ok "contexto fbdev_mali"; else bad "RetroArch sem fbdev_mali"; fi
	grep -q 'video_context_driver = "fbdev_mali"' "$T/etc/brisa/retroarch/retroarch.cfg" &&
		ok "retroarch.cfg usa fbdev_mali" || bad "retroarch.cfg não usa fbdev_mali"
else
	echo "(sem RetroArch: build sem GPU)"
fi

echo "== bibliotecas"
needs() {
	local lib
	for lib in $($RE -d "$1" 2>/dev/null | sed -n 's/.*Shared library: \[\(.*\)\]/\1/p'); do
		[ -e "$T/usr/lib/$lib" ] || [ -e "$T/lib/$lib" ] || bad "$(basename "$1") precisa de $lib"
	done
}
for f in "$T"/usr/bin/brisa-ui "$T"/usr/bin/retroarch "$T"/usr/lib/libmali.so.* "$T"/usr/lib/libretro/*.so; do
	[ -f "$f" ] && ! [ -L "$f" ] && needs "$f"
done
[ $fail = 0 ] && ok "todas as dependências resolvidas"

echo "== vendor"
if [ -f "$T/etc/brisa/hw.conf" ]; then
	. "$T/etc/brisa/hw.conf"
	check "lib/modules/$KVER/modules.dep"
	for m in $MODULES_EARLY $MODULES_LATE $WIFI_MODULES; do
		grep -qE "(^|/)$m\.ko:" "$T/lib/modules/$KVER/modules.dep" && ok "módulo $m" || bad "módulo $m fora do modules.dep"
	done
	[ -n "$(ls "$T"/usr/lib/libmali.so* 2>/dev/null)" ] && ok "libmali" || bad "falta libmali"
	echo "       firmware: $(find "$T/lib/firmware" -type f 2>/dev/null | wc -l) arquivos," \
		"perfis de controle: $(ls "$T/usr/share/brisa/autoconfig" 2>/dev/null | wc -l)"
else
	echo "(sem vendor: rode 'make vendor STOCK=<imagem>')"
fi

echo "== tamanho: $(du -sh "$T" | cut -f1)"
[ $fail = 0 ] && echo "RESULTADO: tudo certo" || echo "RESULTADO: há problemas acima"
exit $fail
