#!/usr/bin/env bash
# make-stock-base.sh <imagem-stock> [diretório-vendor]
#
# Guarda em vendor/stock-base.img.xz uma cópia compacta da imagem stock sem o
# rootfs, que o Brisa substitui de qualquer forma. Bootloader, kernel, tabela de
# partições e demais partições ficam; o rootfs vira zeros e o xz quase o elimina.
# Assim o build-image.sh não depende mais da imagem original de ~15 GB.
set -euo pipefail

die() { echo "make-stock-base: $*" >&2; exit 1; }

[ $# -ge 1 ] || die "uso: $0 <imagem-stock> [diretório-vendor]"
IMG=$1
TOP=$(cd "$(dirname "$0")/.." && pwd)
VENDOR=$(realpath -m "${2:-$TOP/vendor}")
[ -f "$VENDOR/layout.env" ] || die "falta $VENDOR/layout.env (rode o extract-vendor.sh antes)"
# shellcheck source=/dev/null
. "$VENDOR/layout.env"

WORK=${BRISA_WORK:-$HOME/brisa-work}
mkdir -p "$WORK"
TMP=$(mktemp "$WORK/stock-base.XXXXXX")
trap 'rm -f "$TMP"' EXIT

echo "==> copiando a imagem stock sem o rootfs"
dd if="$IMG" of="$TMP" bs=4M conv=sparse status=none
fallocate --punch-hole --keep-size \
	--offset $((ROOTFS_START * 512)) --length $((ROOTFS_SECTORS * 512)) "$TMP"

echo "==> compactando (xz)"
xz -T0 -c "$TMP" > "$VENDOR/stock-base.img.xz"

# caminho relativo ao vendor/: continua valendo se o projeto mudar de pasta
sed -i 's|^STOCK_IMG=.*|STOCK_IMG="stock-base.img.xz"|' "$VENDOR/layout.env"
echo "==> pronto: $VENDOR/stock-base.img.xz ($(du -h "$VENDOR/stock-base.img.xz" | cut -f1))"
