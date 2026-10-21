#!/usr/bin/env bash
# Teste de ponta a ponta de extract-vendor.sh e build-image.sh, sem o
# firmware real: cria uma imagem "stock" falsa com o mesmo formato
# (GPT, boot, rootfs ext4 com módulos/libmali/RetroArch, dados FAT),
# extrai, monta a imagem do Brisa e confere o resultado.
set -euo pipefail
TOP=$(cd "$(dirname "$0")/../.." && pwd)
T=$(mktemp -d "${TMPDIR:-/tmp}/brisa-test.XXXXXX")
trap 'rm -rf "$T"' EXIT
fail() { echo "FALHOU: $*" >&2; exit 1; }
ok() { echo "ok - $*"; }

# ---------- imagem stock falsa ----------
R="$T/stockroot"
mkdir -p "$R/lib/modules/4.9.170/kernel/drivers/net/wireless" "$R/etc/modules-load.d" \
	"$R/usr/lib/aarch64-linux-gnu" "$R/mnt/vendor/deep/retro/autoconfig/udev" "$R/lib/firmware" \
	"$R/proc" "$R/sbin"
: > "$R/lib/modules/4.9.170/kernel/drivers/net/wireless/8821cs.ko"
: > "$R/lib/modules/4.9.170/modules.dep"
printf '# módulos\nmali_kbase\n8821cs\n' > "$R/etc/modules"
printf 'sunxi_gpadc\n' > "$R/etc/modules-load.d/joy.conf"
cp /lib/x86_64-linux-gnu/libz.so.1 "$R/usr/lib/aarch64-linux-gnu/libmali.so.0"
ln -s libmali.so.0 "$R/usr/lib/aarch64-linux-gnu/libmali.so"
printf 'fw' > "$R/lib/firmware/rtl8821cs_fw.bin"
printf 'video_driver = "gl"\nvideo_context_driver = "mali-fbdev"\ninput_exit_emulator_btn = "6"\ninput_player1_a_btn = "1"\n' \
	> "$R/mnt/vendor/deep/retro/retroarch.cfg"
printf 'input_device = "H700 Gamepad"\n' > "$R/mnt/vendor/deep/retro/autoconfig/udev/H700.cfg"

STOCK="$T/stock.img"
truncate -s 360M "$STOCK"
sfdisk -q "$STOCK" <<EOF
label: gpt
start=2048, size=16384, name="boot", type=EBD0A0A2-B9E5-4433-87C0-68B6B72699C7
start=18432, size=2048, name="env"
start=20480, size=524288, name="rootfs"
start=544768, size=174080, name="ROMS", type=EBD0A0A2-B9E5-4433-87C0-68B6B72699C7
EOF
mkfs.vfat -C "$T/boot.img" 8192 >/dev/null
dd if="$T/boot.img" of="$STOCK" bs=512 seek=2048 conv=notrunc status=none
printf 'bootargs=console=ttyS0,115200 root=/dev/mmcblk0p3 rootwait init=/init\0' > "$T/env.img"
dd if="$T/env.img" of="$STOCK" bs=512 seek=18432 conv=notrunc status=none
fakeroot mke2fs -q -t ext4 -L rootfs -U 11111111-2222-3333-4444-555555555555 -d "$R" "$T/root.img" 65536
dd if="$T/root.img" of="$STOCK" bs=512 seek=20480 conv=notrunc status=none
mkfs.vfat -F 32 -n ROMS -C "$T/roms.img" 87040 >/dev/null
dd if="$T/roms.img" of="$STOCK" bs=512 seek=544768 conv=notrunc status=none
gzip -k "$STOCK"
ok "imagem stock falsa criada"

# ---------- extração ----------
V="$T/vendor"
BRISA_WORK="$T/work" "$TOP/scripts/extract-vendor.sh" "$STOCK.gz" "$V" > "$T/extract.log" ||
	{ cat "$T/extract.log"; fail "extract-vendor.sh"; }
. "$V/layout.env"
. "$V/hw.conf"
[ "$ROOTFS_PART" = 3 ] || fail "rootfs deveria ser p3 (veio $ROOTFS_PART)"
[ "$ROOTFS_START" = 20480 ] || fail "início do rootfs"
[ "$ROOTFS_UUID" = 11111111-2222-3333-4444-555555555555 ] || fail "UUID do rootfs"
[ "$STORAGE_PART" = 4 ] || fail "partição de dados deveria ser p4"
[ "$KVER" = 4.9.170 ] || fail "versão do kernel"
[ -f "$V/modules/4.9.170/kernel/drivers/net/wireless/8821cs.ko" ] || fail "módulos"
[ -L "$V/mali/libmali.so" ] && [ -f "$V/mali/libmali.so.0" ] || fail "libmali e link"
[ -f "$V/firmware/rtl8821cs_fw.bin" ] || fail "firmware"
[ -f "$V/autoconfig/H700.cfg" ] || fail "autoconfig"
grep -q 'input_exit_emulator_btn' "$V/retroarch/input-stock.cfg" || fail "input-stock.cfg"
[ "$WIFI_MODULES" = "8821cs " ] || fail "driver Wi-Fi (veio '$WIFI_MODULES')"
case "$MODULES_EARLY" in *mali_kbase*sunxi_gpadc*) ;; *) fail "módulos de boot (veio '$MODULES_EARLY')" ;; esac
grep -q 'root=/dev/mmcblk0p3' "$V/info/report.txt" || fail "cmdline no relatório"
ok "extract-vendor.sh encontrou rootfs, módulos, Mali, firmware, controles e cmdline"

# ---------- montagem ----------
# ROOTFS_TAR=<rootfs.tar do Buildroot> testa com o sistema real.
if [ -n "${ROOTFS_TAR:-}" ]; then
	TARFILE=$ROOTFS_TAR
	VERSION=$(tar -xOf "$TARFILE" ./etc/brisa/version)
else
	mkdir -p "$T/newroot/etc/brisa" "$T/newroot/sbin"
	echo teste > "$T/newroot/etc/brisa/version"
	tar -C "$T/newroot" -cf "$T/rootfs.tar" .
	TARFILE=$T/rootfs.tar
	VERSION=teste
fi
BRISA_WORK="$T" BRISA_VENDOR="$V" "$TOP/scripts/build-image.sh" "$TARFILE" "$T/brisa.img" --wipe-storage > "$T/build.log" ||
	{ cat "$T/build.log"; fail "build-image.sh"; }

cmp -n $((20480 * 512)) "$STOCK" "$T/brisa.img" || fail "boot/env/tabela deveriam ficar iguais"
dd if="$T/brisa.img" of="$T/new-root.img" bs=512 skip=20480 count=524288 status=none
[ "$(blkid -p -o value -s UUID "$T/new-root.img")" = 11111111-2222-3333-4444-555555555555 ] || fail "UUID preservado"
[ "$(blkid -p -o value -s LABEL "$T/new-root.img")" = rootfs ] || fail "rótulo preservado"
[ "$(debugfs -R 'cat /etc/brisa/version' "$T/new-root.img" 2>/dev/null)" = "$VERSION" ] || fail "conteúdo do rootfs novo"
if [ -n "${ROOTFS_TAR:-}" ]; then
	for f in /usr/bin/brisa-ui /etc/init.d/rcS /usr/lib/libretro/snes9x_libretro.so; do
		debugfs -R "stat $f" "$T/new-root.img" 2>/dev/null | grep -q 'Type: regular' || fail "falta $f"
	done
	debugfs -R 'stat /init' "$T/new-root.img" 2>/dev/null | grep -q 'Type: symlink' || fail "/init"
	debugfs -R 'stat /etc/shadow' "$T/new-root.img" 2>/dev/null | grep -q 'Mode:  *0600' || fail "permissão do shadow"
	ok "rootfs real: interface, rcS, cores, /init e permissões no lugar"
fi
debugfs -R 'stat /etc/brisa/version' "$T/new-root.img" 2>/dev/null | grep -q 'User: *0 *Group: *0' || fail "dono root"
[ "$(blkid -p -O $((544768 * 512)) -o value -s LABEL "$T/brisa.img")" = BRISA ] || fail "partição de dados formatada"
ok "build-image.sh trocou só o rootfs, preservou UUID/rótulo e formatou os dados"
[ -n "${VERBOSE:-}" ] && cat "$V/info/report.txt"
echo "TODOS OS TESTES PASSARAM"
