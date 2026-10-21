#!/bin/sh
# Ajustes finais no rootfs, rodado pelo Buildroot depois do overlay.
set -e
TARGET="$1"

# O kernel stock pode chamar /init em vez de /sbin/init.
ln -sf sbin/init "$TARGET/init"

# Chaves do dropbear ficam no rootfs (o Buildroot aponta para /var/run por padrão).
if [ -L "$TARGET/etc/dropbear" ]; then
	rm "$TARGET/etc/dropbear"
fi
mkdir -p "$TARGET/etc/dropbear"

mkdir -p "$TARGET/mnt/sd1" "$TARGET/mnt/sd2" "$TARGET/data"

# Os scripts S* padrão do Buildroot não são usados: o rcS do Brisa faz tudo.
rm -f "$TARGET"/etc/init.d/S[0-9]*

chmod 0755 "$TARGET/etc/init.d/rcS" "$TARGET/etc/init.d/rcK" "$TARGET"/usr/bin/brisa-*

echo "$(date +%Y.%m.%d)" > "$TARGET/etc/brisa/version"
