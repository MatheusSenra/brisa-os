#!/usr/bin/env bash
# extract-vendor.sh <imagem-stock> [diretório-vendor]
#
# Lê a imagem do firmware stock da Anbernic (sem root e sem montar nada) e
# separa o que o Brisa reaproveita:
#   vendor/modules/     módulos do kernel 4.9 (/lib/modules/<versão>)
#   vendor/firmware/    firmware do Wi-Fi/Bluetooth
#   vendor/mali/        libmali (EGL/GLES da Mali-G31)
#   vendor/autoconfig/  perfis de controle do RetroArch
#   vendor/retroarch/   linhas de controle/atalhos do retroarch.cfg stock
#   vendor/hw.conf      módulos a carregar e driver do Wi-Fi
#   vendor/layout.env   partições da imagem, usado por build-image.sh
#   vendor/info/        relatório para conferir o que foi encontrado
#
# Aceita .img, .img.gz, .img.xz, .zip e .7z.
set -euo pipefail

die() { echo "extract-vendor: $*" >&2; exit 1; }
log() { echo "==> $*"; }

[ $# -ge 1 ] || die "uso: $0 <imagem-stock> [diretório-vendor]"
SRC=$(realpath "$1")
TOP=$(cd "$(dirname "$0")/.." && pwd)
VENDOR=$(realpath -m "${2:-$TOP/vendor}")
WORK=${BRISA_WORK:-$HOME/brisa-work}/stock
for t in sfdisk blkid debugfs readelf strings; do
	command -v "$t" >/dev/null || die "falta '$t' (rode 'make deps')"
done

mkdir -p "$WORK"
rm -rf "$VENDOR"
mkdir -p "$VENDOR/info"
REPORT="$VENDOR/info/report.txt"
: > "$REPORT"
rep() { echo "$*" | tee -a "$REPORT"; }

# ---------- 1. imagem descompactada ----------
IMG="$WORK/stock.img"
case "$SRC" in
*.gz) log "descompactando gzip"; gzip -dc "$SRC" > "$IMG" ;;
*.xz) log "descompactando xz"; xz -dc "$SRC" > "$IMG" ;;
*.zip) log "descompactando zip"; unzip -p "$SRC" '*.img' > "$IMG" ;;
*.7z) log "descompactando 7z"; 7z e -so "$SRC" '*.img' > "$IMG" ;;
*) IMG="$SRC" ;;
esac
[ -s "$IMG" ] || die "imagem vazia: $IMG"

# ---------- 2. partições ----------
log "lendo a tabela de partições"
sfdisk -d "$IMG" > "$VENDOR/info/partitions.sfdisk" || die "tabela de partições ilegível"
cp "$VENDOR/info/partitions.sfdisk" "$WORK/"

declare -a P_NUM P_START P_SIZE P_TYPE P_NAME
while IFS= read -r line; do
	case "$line" in *": start="*) ;; *) continue ;; esac
	dev=${line%% :*}
	num=${dev##*[!0-9]}
	start=$(sed -n 's/.*start= *\([0-9]*\).*/\1/p' <<<"$line")
	size=$(sed -n 's/.*size= *\([0-9]*\).*/\1/p' <<<"$line")
	name=$(sed -n 's/.*name="\([^"]*\)".*/\1/p' <<<"$line")
	type=$(blkid -p -O $((start * 512)) -o value -s TYPE "$IMG" 2>/dev/null || true)
	P_NUM+=("$num"); P_START+=("$start"); P_SIZE+=("$size")
	P_TYPE+=("${type:--}"); P_NAME+=("${name:--}")
done < "$VENDOR/info/partitions.sfdisk"
[ ${#P_NUM[@]} -gt 0 ] || die "nenhuma partição encontrada"

rep "Partições de $(basename "$SRC"):"
for i in "${!P_NUM[@]}"; do
	rep "$(printf '  p%-2s %-10s %-8s %8d MB  início %d' "${P_NUM[$i]}" "${P_NAME[$i]}" \
		"${P_TYPE[$i]}" $((P_SIZE[$i] / 2048)) "${P_START[$i]}")"
done

part_file() { # <índice> -> extrai a partição para um arquivo
	local f="$WORK/p${P_NUM[$1]}.img"
	[ -f "$f" ] || dd if="$IMG" of="$f" bs=512 skip="${P_START[$1]}" count="${P_SIZE[$1]}" \
		conv=sparse status=none
	echo "$f"
}

# ---------- 3. rootfs ----------
ROOT=
for i in "${!P_NUM[@]}"; do
	case "${P_TYPE[$i]}" in ext2|ext3|ext4) ;; *) continue ;; esac
	f=$(part_file "$i")
	mods=$(debugfs -R 'ls -p /lib/modules' "$f" 2>/dev/null; debugfs -R 'ls -p /usr/lib/modules' "$f" 2>/dev/null)
	if grep -q '^/' <<<"$mods"; then
		ROOT=$i
		break
	fi
done
[ -n "$ROOT" ] || die "não achei a partição do rootfs (ext4 com /lib/modules)"
ROOT_IMG=$(part_file "$ROOT")
eval "$(blkid -p -o export "$ROOT_IMG" | grep -E '^(UUID|LABEL|TYPE)=' | sed 's/^/ROOTFS_/')"
rep ""
rep "Rootfs: p${P_NUM[$ROOT]} (${ROOTFS_TYPE:-?}, rótulo '${ROOTFS_LABEL:-}', UUID ${ROOTFS_UUID:-?})"

log "copiando o rootfs stock (só leitura, via debugfs)"
RFS="$WORK/rootfs"
rm -rf "$RFS"
mkdir -p "$RFS"
debugfs -R 'ls -p /' "$ROOT_IMG" 2>/dev/null | awk -F/ '$3 ~ /^04/ { print $6 }' |
	grep -vxE '\.|\.\.|proc|sys|dev|run|tmp|lost\+found' |
	while read -r d; do
		debugfs -R "rdump /$d $RFS" "$ROOT_IMG" > /dev/null 2>> "$WORK/rdump.log" || true
	done

# ---------- 4. módulos do kernel ----------
MODDIR=
for d in "$RFS/lib/modules" "$RFS/usr/lib/modules"; do
	[ -d "$d" ] && MODDIR=$d && break
done
KVER=$(ls "$MODDIR" | head -n1)
mkdir -p "$VENDOR/modules"
cp -a "$MODDIR/$KVER" "$VENDOR/modules/"
NKO=$(find "$VENDOR/modules" -name '*.ko' | wc -l)
rep "Kernel: $KVER ($NKO módulos)"

# ---------- 5. firmware ----------
for d in "$RFS/lib/firmware" "$RFS/usr/lib/firmware" "$RFS/vendor/etc/firmware"; do
	[ -d "$d" ] || continue
	mkdir -p "$VENDOR/firmware"
	cp -a "$d/." "$VENDOR/firmware/"
done
rep "Firmware: $(find "$VENDOR/firmware" -type f 2>/dev/null | wc -l) arquivos"

# ---------- 6. Mali ----------
mkdir -p "$VENDOR/mali"
find "$RFS" \( -name 'libmali*' -o -name 'libMali*' \) \( -type f -o -type l \) 2>/dev/null |
	while read -r f; do cp -a "$f" "$VENDOR/mali/"; done
rep ""
if ls "$VENDOR"/mali/* >/dev/null 2>&1; then
	rep "Mali: $(cd "$VENDOR/mali" && ls | tr '\n' ' ')"
	blob=$(find "$VENDOR/mali" -type f | head -n1)
	needed=$(readelf -d "$blob" 2>/dev/null | sed -n 's/.*Shared library: \[\(.*\)\]/\1/p' | tr '\n' ' ')
	rep "  depende de: $needed"
	syms=$(readelf --dyn-syms -W "$blob" 2>/dev/null)
	if grep -q ' wl_' <<<"$syms"; then rep "  variante: Wayland (o RetroArch precisaria de outro contexto)"
	elif grep -q ' gbm_' <<<"$syms"; then rep "  variante: GBM/DRM (o RetroArch precisaria de --enable-kms)"
	else rep "  variante: provavelmente fbdev (combina com o RetroArch mali-fbdev)"; fi
else
	rep "Mali: NENHUMA libmali encontrada. Bibliotecas EGL/GLES presentes:"
	find "$RFS" -name 'libEGL*' -o -name 'libGLES*' 2>/dev/null | sed "s|$RFS||; s|^|  |" | tee -a "$REPORT"
fi

# ---------- 7. RetroArch stock ----------
mkdir -p "$VENDOR/retroarch"
RACFG=$(grep -rl --include='retroarch*.cfg' 'video_driver' "$RFS" 2>/dev/null | head -n1 || true)
if [ -n "$RACFG" ]; then
	cp "$RACFG" "$VENDOR/retroarch/stock-retroarch.cfg"
	grep -E '^input_(enable_hotkey|exit_emulator|menu_toggle|save_state|load_state|state_slot_(increase|decrease)|fps_toggle|player1_)' \
		"$RACFG" > "$VENDOR/retroarch/input-stock.cfg" || true
	rep ""
	rep "RetroArch stock: ${RACFG#$RFS}"
	{ grep -E '^(video_driver|video_context_driver|input_joypad_driver|audio_driver) ' "$RACFG" || true; } |
		sed 's/^/  /' | tee -a "$REPORT"
fi
AUTOCONF=$(find "$RFS" -type d -path '*autoconfig*' -name 'udev' 2>/dev/null | head -n1 || true)
if [ -n "$AUTOCONF" ]; then
	mkdir -p "$VENDOR/autoconfig"
	cp -a "$AUTOCONF/." "$VENDOR/autoconfig/"
	rep "Perfis de controle: $(ls "$VENDOR/autoconfig" | wc -l) (${AUTOCONF#$RFS})"
fi

# ---------- 8. módulos carregados no boot ----------
{
	grep -hvE '^\s*(#|$)' "$RFS/etc/modules" "$RFS"/etc/modules-load.d/*.conf 2>/dev/null || true
	# scripts de boot do stock que chamam insmod/modprobe
	grep -rhoE '(insmod|modprobe)[[:space:]]+[^[:space:];|&]+' \
		"$RFS/etc/init.d" "$RFS/etc/rc.local" "$RFS/usr/local/bin" "$RFS/etc/systemd" 2>/dev/null |
		awk '{ print $2 }' || true
} | sed 's|.*/||; s|\.ko$||' | awk '!seen[$0]++' > "$VENDOR/info/modules-stock.txt"

WIFI_RE='^(8821cs|8723ds|8723bs|8189fs|8189es|88x2cs|8822cs|8852bs|aic8800_fdrv|bcmdhd|xr829|ssv6x5x|rtl8.*)$'
EARLY=$({ grep -vE "$WIFI_RE|^(aic8800_bsp|bt.*|hci.*|rfcomm|bluetooth)$" "$VENDOR/info/modules-stock.txt" || true; } | tr '\n' ' ')
WIFI=$({ grep -E "$WIFI_RE" "$VENDOR/info/modules-stock.txt" || true; } | tr '\n' ' ')
if [ -z "$WIFI" ]; then
	WIFI=$(find "$VENDOR/modules" -name '*.ko' | sed 's|.*/||; s|\.ko$||' | grep -E "$WIFI_RE" | head -n1 || true)
fi
rep ""
rep "Módulos carregados pelo stock: ${EARLY:-nenhum}"
rep "Driver Wi-Fi: ${WIFI:-não identificado}"

# ---------- 9. linha de comando do kernel ----------
rep ""
rep "Linha de comando do kernel (procurada nas partições de boot):"
for i in "${!P_NUM[@]}"; do
	[ "$i" = "$ROOT" ] && continue
	[ "${P_SIZE[$i]}" -lt $((256 * 2048)) ] || continue
	{ strings -n 12 "$(part_file "$i")" | grep -E 'root=|console=' || true; } | head -n3 |
		sed "s/^/  p${P_NUM[$i]}: /" | tee -a "$REPORT"
done

# ---------- 10. partição de dados ----------
STORE=
best=0
for i in "${!P_NUM[@]}"; do
	case "${P_TYPE[$i]}" in vfat|exfat) ;; *) continue ;; esac
	if [ "${P_SIZE[$i]}" -gt "$best" ]; then STORE=$i; best=${P_SIZE[$i]}; fi
done

cat > "$VENDOR/hw.conf" <<EOF
# Gerado por extract-vendor.sh a partir de $(basename "$SRC")
KVER="$KVER"
MODULES_EARLY="$EARLY"
MODULES_LATE=""
WIFI_MODULES="$WIFI"
EOF

cat > "$VENDOR/layout.env" <<EOF
STOCK_IMG="$IMG"
KVER="$KVER"
ROOTFS_PART=${P_NUM[$ROOT]}
ROOTFS_START=${P_START[$ROOT]}
ROOTFS_SECTORS=${P_SIZE[$ROOT]}
ROOTFS_TYPE="${ROOTFS_TYPE:-ext4}"
ROOTFS_LABEL="${ROOTFS_LABEL:-}"
ROOTFS_UUID="${ROOTFS_UUID:-}"
STORAGE_PART=${STORE:+${P_NUM[$STORE]}}
STORAGE_START=${STORE:+${P_START[$STORE]}}
STORAGE_SECTORS=${STORE:+${P_SIZE[$STORE]}}
STORAGE_TYPE="${STORE:+${P_TYPE[$STORE]}}"
EOF

rep ""
rep "Pronto. Vendor em $VENDOR"
rep "Confira $REPORT antes de compilar."
