#!/usr/bin/env bash
# build-image.sh <rootfs.tar> <saída.img> [--wipe-storage] [--xz]
#
# Monta a imagem do Brisa a partir da imagem stock: mantém bootloader,
# kernel e tabela de partições da Anbernic e troca só o rootfs.
#   --wipe-storage  formata a partição de dados (FAT32, rótulo BRISA)
#   --xz            gera também <saída>.img.xz
# Não precisa de root: usa fakeroot e mke2fs -d.
set -euo pipefail

die() { echo "build-image: $*" >&2; exit 1; }
log() { echo "==> $*"; }

[ $# -ge 2 ] || die "uso: $0 <rootfs.tar> <saída.img> [--wipe-storage] [--xz]"
TAR=$(realpath "$1")
OUT=$(realpath -m "$2")
shift 2
WIPE=0
XZ=0
for a in "$@"; do
	case "$a" in
	--wipe-storage) WIPE=1 ;;
	--xz) XZ=1 ;;
	*) die "opção desconhecida: $a" ;;
	esac
done

TOP=$(cd "$(dirname "$0")/.." && pwd)
LAYOUT=${BRISA_VENDOR:-$TOP/vendor}/layout.env
[ -f "$LAYOUT" ] || die "falta $LAYOUT; rode 'make vendor STOCK=<imagem>' antes"
# shellcheck source=/dev/null
. "$LAYOUT"
[ -f "$STOCK_IMG" ] || die "imagem stock não encontrada: $STOCK_IMG"
[ -f "$TAR" ] || die "rootfs não encontrado: $TAR"

TMP=$(mktemp -d "${BRISA_WORK:-$HOME/brisa-work}/img.XXXXXX")
trap 'rm -rf "$TMP"' EXIT

# ---------- rootfs ext4 do tamanho exato da partição ----------
SIZE_KB=$((ROOTFS_SECTORS / 2))
FEATURES=
# orphan_file (e2fsprogs >= 1.47) não existe no kernel 4.9
if mke2fs -O orphan_file -n -F "$TMP/probe" 1024 >/dev/null 2>&1; then
	FEATURES="-O ^orphan_file"
fi
rm -f "$TMP/probe"

log "criando o ext4 do rootfs (${SIZE_KB} KB)"
# shellcheck disable=SC2016
fakeroot -- sh -c '
	set -e
	mkdir "$1/root"
	tar -xpf "$2" -C "$1/root"
	used=$(du -sk "$1/root" | cut -f1)
	if [ "$used" -gt $(( $3 * 9 / 10 )) ]; then
		echo "build-image: rootfs (${used} KB) não cabe na partição (${3} KB)" >&2
		exit 1
	fi
	mke2fs -q -F -t ext4 $4 ${5:+-L "$5"} ${6:+-U "$6"} -d "$1/root" "$1/rootfs.ext4" "$3"
' sh "$TMP" "$TAR" "$SIZE_KB" "$FEATURES" "$ROOTFS_LABEL" "$ROOTFS_UUID"

# ---------- imagem final ----------
log "copiando a imagem stock"
cp --sparse=always "$STOCK_IMG" "$OUT"
log "gravando o rootfs na partição $ROOTFS_PART"
dd if="$TMP/rootfs.ext4" of="$OUT" bs=512 seek="$ROOTFS_START" conv=notrunc,sparse status=none

if [ "$WIPE" = 1 ]; then
	[ -n "${STORAGE_PART:-}" ] || die "a imagem stock não tem partição de dados"
	log "formatando a partição de dados $STORAGE_PART (FAT32, BRISA)"
	mkfs.vfat -F 32 -n BRISA -C "$TMP/storage.img" $((STORAGE_SECTORS / 2)) >/dev/null
	dd if="$TMP/storage.img" of="$OUT" bs=512 seek="$STORAGE_START" conv=notrunc,sparse status=none
fi

if [ "$XZ" = 1 ]; then
	log "compactando"
	xz -T0 -k -f "$OUT"
fi

log "pronto: $OUT ($(du -h --apparent-size "$OUT" | cut -f1))"
