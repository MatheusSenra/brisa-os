# Diagnóstico do volume do Brisa OS.
# Copie para Brisa/dev/boot.sh no cartão; a saída vai para Brisa/logs/dev-boot.log.

echo "== $(date)"
ls -l /usr/bin/brisa-volume /usr/bin/amixer 2>&1
echo "== controles de volume da placa 0 (com faixa em dB)"
amixer -c 0 cget name='lineout volume' 2>&1
amixer -c 0 cget name='digital volume' 2>&1

echo "== brisa-volume get, passo a passo"
sh -x /usr/bin/brisa-volume get 2>&1; echo "rc=$?"
echo "== brisa-volume up, passo a passo"
sh -x /usr/bin/brisa-volume up 2>&1; echo "rc=$?"
echo "== do jeito que a interface chama (popen)"
sh -c "'/usr/bin/brisa-volume' down 2>/dev/null"; echo "rc=$?"

echo "== volume depois dos testes"
amixer -c 0 cget name='lineout volume' 2>&1 | grep ': values'
echo "== variáveis de áudio"
. /usr/lib/brisa/common.sh
echo "AUDIO_CARD=$AUDIO_CARD AUDIO_CONTROL=$AUDIO_CONTROL"
