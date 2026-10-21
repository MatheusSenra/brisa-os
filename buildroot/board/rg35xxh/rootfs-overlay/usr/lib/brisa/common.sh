# Funções e caminhos comuns aos scripts do Brisa OS (incluir com ".").

BRISA_ETC=/etc/brisa
[ -f /run/brisa/env ] && . /run/brisa/env
: "${BRISA_DATA:=/mnt/sd1/Brisa}"
: "${BRISA_ROMS:=/mnt/sd1/Roms}"
export BRISA_ETC BRISA_DATA BRISA_ROMS

# hw.conf vem do firmware stock (scripts/extract-vendor.sh).
[ -f "$BRISA_ETC/hw.conf" ] && . "$BRISA_ETC/hw.conf"
: "${WIFI_MODULES:=8821cs}"
: "${WIFI_IFACE:=wlan0}"
# Som do RG35XX H (placa 0, "audiocodec"): DAC -> mixers de saída -> LINEOUT -> amplificador.
: "${AUDIO_CARD:=0}"
: "${AUDIO_CONTROL:=lineout volume}"
: "${AUDIO_ROUTE:=OutputL Mixer DACL Switch|OutputR Mixer DACR Switch|LINEOUT Switch|SPK Switch}"

# brisa_setting <chave> <padrão>
brisa_setting() {
	v=$(sed -n "s/^$1=//p" "$BRISA_DATA/settings.conf" 2>/dev/null | tail -n1 | tr -d '\r')
	echo "${v:-$2}"
}

# Governor que acompanha a carga, entre os que o kernel oferece.
brisa_gov_dynamic() {
	avail=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors 2>/dev/null)
	for g in schedutil ondemand interactive conservative; do
		case " $avail " in
		*" $g "*) echo "$g"; return ;;
		esac
	done
	echo performance
}

# Liga o caminho do som até o alto-falante. Sem ele o driver da Allwinner não
# inicia a reprodução e o RetroArch, que sincroniza pelo áudio, trava no 1º quadro.
brisa_audio_route() {
	echo "$AUDIO_ROUTE" | tr '|' '\n' | while IFS= read -r c; do
		[ -n "$c" ] && amixer -q -c "$AUDIO_CARD" cset name="$c" on 2>/dev/null
	done
}

brisa_set_governor() {
	for f in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
		[ -w "$f" ] && echo "$1" > "$f"
	done 2>/dev/null
}

# Copia a configuração padrão do RetroArch para o cartão na primeira vez,
# trocando @DATA@ pela pasta de dados real.
brisa_retroarch_prepare() {
	d="$BRISA_DATA/retroarch"
	mkdir -p "$d"
	for f in retroarch.cfg retroarch-core-options.cfg; do
		[ -f "$d/$f" ] || sed "s|@DATA@|$BRISA_DATA|g" "$BRISA_ETC/retroarch/$f" > "$d/$f"
	done
}
