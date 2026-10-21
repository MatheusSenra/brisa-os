# Diagnóstico de áudio do Brisa OS.
# Copie para Brisa/dev/boot.sh no cartão; a saída vai para Brisa/logs/dev-boot.log.
# Cada teste de reprodução tem limite de tempo, então nada trava o boot.

echo "== $(date) / $(uname -r)"
echo "== placas"
cat /proc/asound/cards
echo "== dispositivos PCM"
cat /proc/asound/pcm
echo "== módulos de som carregados"
lsmod 2>/dev/null | grep -iE 'snd|sound|codec|audio' || echo "(nenhum módulo; som embutido no kernel)"

echo "== mixer da placa 0"
amixer -c 0 contents 2>&1 | grep -E "name=|: values="

# 2 s de silêncio em cada dispositivo. rc=0: tocou; rc=143/124: travou (morto pelo timeout).
for dev in default plughw:0,0 hw:0,0 plughw:2,0; do
	printf '== tocar em %-12s ' "$dev"
	timeout 8 aplay -q -D "$dev" -f S16_LE -r 48000 -c 2 -d 2 /dev/zero 2>&1
	echo "rc=$?"
done

echo "== status da reprodução na placa 0"
cat /proc/asound/card0/pcm0p/sub0/status 2>&1
cat /proc/asound/card0/pcm0p/sub0/hw_params 2>&1

echo "== kernel: mensagens de som"
dmesg | grep -iE 'snd|sound|codec|audio|i2s|ahub|dma|spk|amp' | tail -n 60
echo "== kernel: últimas mensagens"
dmesg | tail -n 40
