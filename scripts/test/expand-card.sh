#!/usr/bin/env bash
# Testa o brisa-expand com os binários ARM do Brisa (via qemu-user) num cartão
# falso no layout da Anbernic: GPT de 8 entradas, 7 partições, imagem menor que
# o cartão (cabeçalho GPT reserva no fim da imagem, não do cartão).
#
# uso: scripts/test/expand-card.sh [output-do-buildroot]   (precisa de 'make rootfs')
set -euo pipefail
TOP=$(cd "$(dirname "$0")/../.." && pwd)
TGT=${1:-$HOME/brisa-work/output}/target
EXPAND=$TOP/buildroot/board/rg35xxh/rootfs-overlay/usr/sbin/brisa-expand
command -v qemu-aarch64-static > /dev/null || { echo "instale qemu-user-static" >&2; exit 1; }
[ -x "$TGT/bin/busybox" ] || { echo "rootfs não encontrado em $TGT" >&2; exit 1; }

T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
fail() { echo "FALHOU: $*" >&2; exit 1; }
ok() { echo "ok - $*"; }

# Comandos do Brisa (ARM) no PATH, rodando pelo qemu.
mkdir -p "$T/bin"
arm() { printf '#!/bin/sh\nexec qemu-aarch64-static -L %s %s "$@"\n' "$TGT" "$2" > "$T/bin/$1"; chmod +x "$T/bin/$1"; }
for a in sh awk sed grep head; do arm "$a" "$TGT/bin/busybox $a"; done
for b in sfdisk mkfs.exfat; do
	p=$(cd "$TGT" && { ls usr/sbin/$b sbin/$b usr/bin/$b 2>/dev/null || true; } | head -n1)
	[ -n "$p" ] || fail "$b não está no rootfs"
	arm "$b" "$TGT/$p"
done
run() { PATH="$T/bin:/usr/bin:/bin" qemu-aarch64-static -L "$TGT" "$TGT/bin/busybox" sh "$EXPAND" "$@"; }

# Cartão "gravado": imagem de 300 MB no layout da Anbernic.
mkcard() { # <arquivo> <MB da imagem> [partição 8 extra]
	truncate -s "${2}M" "$1"
	{
		echo "label: gpt"
		echo "table-length: 8"
		echo "start=2048, size=8192, name=\"Roms\", type=EBD0A0A2-B9E5-4433-87C0-68B6B72699C7"
		echo "start=10240, size=2048, name=\"boot-resource\""
		echo "start=12288, size=2048, name=\"env\""
		echo "start=14336, size=4096, name=\"boot\""
		echo "start=18432, size=262144, name=\"rootfs\""
		echo "start=280576, size=131072, name=\"appfs\""
		echo "start=411648, size=180224, name=\"UDISK\""
		if [ -n "${3:-}" ]; then echo "start=591872, size=2048, name=\"extra\""; fi
	} | sfdisk -q "$1"
}

C=$T/card.img
mkcard "$C" 300
sfdisk -d "$C" | grep 'start=' > "$T/antes.txt"
truncate -s 1500M "$C" # cartão maior que a imagem

run --check "$C" || fail "--check deveria ver espaço livre"
ok "--check encontrou espaço livre"

out=$(run "$C" 2> "$T/err.txt") || { cat "$T/err.txt"; fail "brisa-expand"; }
[ "$out" = p8 ] || fail "deveria criar p8 (veio '$out')"
sfdisk -d "$C" > "$T/depois.txt"
grep -q 'name="BRISA"' "$T/depois.txt" || fail "partição BRISA"
grep 'name="BRISA"' "$T/depois.txt" | grep -q 'EBD0A0A2-B9E5-4433-87C0-68B6B72699C7' || fail "tipo dados básicos"
grep -v BRISA "$T/depois.txt" | grep 'start=' | cmp -s - "$T/antes.txt" || fail "partições 1-7 mudaram"
read -r start size < <(grep 'name="BRISA"' "$T/depois.txt" | sed -n 's/.*start= *\([0-9]*\), size= *\([0-9]*\).*/\1 \2/p')
[ $((start % 2048)) = 0 ] && [ "$start" -ge $((411648 + 180224)) ] || fail "início da BRISA ($start)"
end=$((start + size))
total=$((1500 * 2048))
[ $((total - end)) -le 2048 ] || fail "BRISA deveria ir até o fim do cartão (sobram $((total - end)) setores)"
sfdisk --verify "$C" 2>&1 | grep -qiE 'problem|corrupt|mismatch' && fail "sfdisk --verify: $(sfdisk --verify "$C" 2>&1)"
ok "p8 BRISA criada até o fim do cartão ($((size / 2048)) MiB), partições 1-7 intactas, GPT reserva no fim"

run "$C" > /dev/null 2>&1 && fail "segunda execução deveria não fazer nada"
ok "segunda execução não faz nada"

mkcard "$T/cheio.img" 300 extra
truncate -s 1500M "$T/cheio.img"
run --check "$T/cheio.img" 2> /dev/null && fail "tabela cheia deveria ser recusada"
ok "tabela de 8 entradas cheia é recusada"

mkcard "$T/justo.img" 300
run --check "$T/justo.img" && fail "cartão do tamanho da imagem não tem o que expandir"
ok "cartão do tamanho da imagem: nada a fazer"

truncate -s 64M "$T/fs.img"
PATH="$T/bin:$PATH" mkfs.exfat -L BRISA "$T/fs.img" > /dev/null || fail "mkfs.exfat ARM"
[ "$(blkid -p -o value -s LABEL "$T/fs.img")" = BRISA ] && [ "$(blkid -p -o value -s TYPE "$T/fs.img")" = exfat ] ||
	fail "exFAT com rótulo BRISA"
ok "mkfs.exfat do Brisa formata exFAT com rótulo BRISA"
echo "TODOS OS TESTES PASSARAM"
