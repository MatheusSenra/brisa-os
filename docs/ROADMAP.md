# Roadmap do Brisa OS

> Plano de evolução do Brisa OS, construído a partir de um estudo do **muOS (MustardOS)** e do **KNULLI**,
> os dois firmwares alternativos mais maduros para os portáteis Allwinner H700 da Anbernic.
> Revisado em outubro de 2026. As fontes estão no fim do documento.

## Sumário

1. [Onde estamos](#1-onde-estamos)
2. [O que o muOS e o KNULLI ensinam](#2-o-que-o-muos-e-o-knulli-ensinam)
3. [Comparativo de recursos](#3-comparativo-de-recursos)
4. [Identidade e diferenciais do Brisa](#4-identidade-e-diferenciais-do-brisa)
5. [Fases do roadmap](#5-fases-do-roadmap)
6. [Engenharia e qualidade (contínuo)](#6-engenharia-e-qualidade-contínuo)
7. [Próximas entregas sugeridas](#7-próximas-entregas-sugeridas)
8. [Riscos e decisões em aberto](#8-riscos-e-decisões-em-aberto)
9. [Fontes](#9-fontes)

---

## 1. Onde estamos

O Brisa já roda no **RG35XX H** real: boot com logo próprio, interface em C desenhada direto no framebuffer, controles, cartão de jogos, RetroArch com GPU Mali-G31 e som, saída do jogo pelo POWER e desligamento limpo.

| Já temos | Ainda não temos |
|---|---|
| Interface própria, leve, em português | Capas, temas, busca, favoritos |
| Boot curto (rootfs montado ~1,8 s após o kernel) | Suspender/ociosidade, Quick Resume completo |
| 6 cores (NES, GB/GBC, GBA, SNES, Sega 8/16-bit e CD, PS1) | Arcade, N64, PSP, NDS, Dreamcast, PortMaster |
| Governor e run-ahead por sistema, "continuar de onde parou" | Ajustes por jogo |
| Wi-Fi por arquivo, SSH/SFTP | Wi-Fi pela interface, painel web, sincronização |
| TF2, expansão para o cartão inteiro (em teste) | Atualização sem regravar o cartão |
| Modo desenvolvedor, logs no cartão, testes automatizados | HDMI, Bluetooth, outros aparelhos H700 |

---

## 2. O que o muOS e o KNULLI ensinam

### muOS (MustardOS)
Firmware **próprio**, com frontend escrito pela equipe, focado em ser *"configurável, tematizável e fácil de usar"*.

- **Navegação de conteúdo** com coleções (com subpastas), histórico, busca e ordenação; menu de opções por item.
- **Ajustes por conteúdo:** escolher o core, o governor da CPU, o esquema de controle e um filtro de cor para cada jogo.
- **Energia:** função do POWER configurável, tempo para apagar a tela e para dormir, governor ocioso separado do padrão, protetor de tela.
- **Serviços web:** SSH, SFTP com **FileBrowser** (gerenciador de arquivos no navegador), terminal web (ttyd), **Syncthing** e **Tailscale**.
- **Visual:** sistema de temas completo, sobreposições de brilho/volume/bateria (inclusive dentro do jogo), filtros de cor com predefinições para daltonismo.
- **Extras:** rastreador de atividade (tempo de jogo, com exportação), capturas de tela, modo console (HDMI), modo quiosque, senha, backup do aparelho, Archive Manager para instalar pacotes, PortMaster, notícias da comunidade.
- **Atalhos padronizados:** MENU + X (menu), MENU + START (sair), MENU + L2/R2 (carregar/salvar estado), MENU + R1 (acelerar).

### KNULLI
**Fork do Batocera** para portáteis de baixo consumo. Herda o ecossistema do Batocera e foi separado para poder incluir kernels binários.

- **Frontend EmulationStation** (ou Simplermenu em aparelhos sem GPU), temas, música de fundo, molduras (*bezels*) e shaders.
- **Capas baixadas no próprio aparelho** (ScreenScraper, TheGamesDB, ArcadeDB): caixa 2D/3D, logos, vídeo de prévia, descrição, manuais.
- **Configuração em camadas:** global → sistema → jogo, com "auto" herdando da camada de cima.
- **Quick Resume:** segurar POWER 2 s salva e desliga; ao ligar, volta direto ao último jogo no mesmo ponto. Apresentado como **substituto do modo suspenso**, porque *suspender drena a bateria nesses aparelhos e não há conserto por software*.
- **Energia em dois estágios:** escurecer/apagar → suspender/desligar após mais tempo ocioso; não economizar enquanto carrega.
- **Rede:** Wi-Fi, Bluetooth (inclusive áudio), SSH, **Syncthing com sincronização automática ao sair do jogo**, netplay, **RetroAchievements**.
- **Extras:** BatteryPlus (bateria mais precisa), LEDs RGB, segundo cartão, migração entre cartões, atualizações, PortMaster, emuladores standalone (Yabasanshiro, PPSSPP), ScummVM, PICO-8.

### Lições para o Brisa
1. **Quick Resume vale mais que suspender** nesses aparelhos: é o caminho para o "liga e já está jogando".
2. **Rede sem fricção** (gerenciar arquivos pelo navegador e sincronizar saves sozinho) é o que o usuário mais sente no dia a dia.
3. **Capas e coleções** são o que faz a biblioteca parecer "pronta".
4. **Configuração em camadas** (global → sistema → jogo) resolve ajustes finos sem poluir a interface.
5. **Os dois cresceram com muitas opções.** Há espaço para um sistema **mais simples, rápido e bem padronizado**, que é a proposta do Brisa.

---

## 3. Comparativo de recursos

Legenda: ✅ tem · 🟡 parcial · ❌ não tem · — não se aplica/sem informação

| Recurso | muOS | KNULLI | Brisa hoje |
|---|---|---|---|
| Frontend próprio e leve | ✅ | ❌ (EmulationStation) | ✅ framebuffer, sem GPU |
| Idioma português | 🟡 | 🟡 | ✅ nativo |
| Temas | ✅ | ✅ | ❌ |
| Capas / mídia dos jogos | ✅ (pacotes) | ✅ (download no aparelho) | ❌ |
| Coleções / favoritos / histórico | ✅ | ✅ | 🟡 recentes |
| Busca | ✅ | ✅ | ❌ |
| Core por jogo | ✅ | ✅ | ❌ (por sistema) |
| Governor por jogo | ✅ | — | 🟡 por sistema |
| Configuração em camadas | 🟡 | ✅ | 🟡 sistema |
| Quick Resume (voltar ao jogo ao ligar) | — | ✅ | 🟡 salva o estado |
| Ociosidade (apagar tela / desligar) | ✅ | ✅ | ❌ |
| Desligamento seguro pelo POWER | ✅ | ✅ | ✅ |
| Wi-Fi pela interface | ✅ | ✅ | ❌ (arquivo) |
| SSH / SFTP | ✅ | ✅ | ✅ |
| Gerenciador de arquivos no navegador | ✅ | 🟡 | ❌ |
| Sincronização de saves | ✅ Syncthing | ✅ Syncthing automático | ❌ |
| RetroAchievements | — | ✅ | ❌ |
| Bluetooth | — | ✅ | ❌ |
| HDMI | ✅ | ✅ | ❌ |
| PortMaster | ✅ | ✅ | ❌ |
| Arcade / N64 / PSP / NDS / DC | ✅ | ✅ | ❌ |
| Atualização sem regravar | ✅ | ✅ | ❌ |
| Estatísticas de tempo de jogo | ✅ | — | ❌ |
| Filtros de cor / acessibilidade | ✅ | 🟡 shaders | ❌ |
| Testes automatizados no build | — | — | ✅ |
| Diagnóstico sem regravar (modo dev) | 🟡 terminal web | 🟡 SSH | ✅ |

---

## 4. Identidade e diferenciais do Brisa

Não vale a pena competir em número de recursos: o muOS e o KNULLI têm anos de vantagem e comunidades grandes. O Brisa deve ser **o firmware mais rápido, mais simples e mais confiável** para quem quer ligar e jogar, sem abrir mão do essencial.

### Princípios
- **Ligar e jogar.** A métrica principal é o tempo do botão POWER até o jogo.
- **Padrões bons, poucas opções.** Cada sistema já vem ajustado e medido no H700; mexer é opcional.
- **Português primeiro**, com inglês como segunda língua, para abrir o projeto à comunidade internacional.
- **Confiável.** Nada de cartão corrompido, nada de travar sem explicação: logs no cartão, testes e diagnóstico.
- **Aberto e reproduzível.** Build com Buildroot, testes automatizados e documentação de como cada peça funciona.

### Diferenciais propostos

| Diferencial | O que é | Por que importa |
|---|---|---|
| **⚡ Instant-on** | Interface na tela em menos de 3 s e Quick Resume ligado por padrão: o POWER leva direto ao jogo onde você parou | É o que mais se sente no uso; nenhum dos dois tem isso como meta medida |
| **🌐 Painel Brisa** | Página web servida pelo aparelho, em português: enviar ROMs arrastando, baixar saves, trocar Wi-Fi, ver logs. O endereço aparece na tela como **QR code** | Substitui SFTP, FileBrowser e app por algo que funciona no celular sem instalar nada |
| **📊 Perfis medidos** | Cada sistema com governor, run-ahead e latência de áudio escolhidos por **medição** (FPS e consumo), publicados numa tabela | Transparência e o melhor equilíbrio entre bateria e desempenho, sem o usuário precisar testar |
| **🛡️ Cartão à prova de falhas** | Verificação e reparo automáticos do FAT/exFAT no boot, gravações atômicas, alerta de bateria crítica com salvamento e desligamento | Ataca o problema que mais dá dor de cabeça nesses aparelhos: cartão corrompido |
| **🔋 Bateria inteligente** | Desliga o Wi-Fi quando ocioso, apaga a tela, desliga sozinho salvando o jogo, e mostra uma estimativa de horas restantes | Mais horas de jogo sem o usuário precisar configurar |
| **🧰 Feito para mexer** | Modo desenvolvedor, logs no cartão, build documentado e testável | Atrai contribuidores e acelera a correção de problemas |

---

## 5. Fases do roadmap

Esforço: **P** (até 1 dia) · **M** (alguns dias) · **G** (semanas)

### Fase 0: Fundamentos ✳️ em andamento
Deixar a base sólida antes de crescer.

| Item | Esforço | Pronto quando |
|---|---|---|
| Expansão para o cartão inteiro | — | Validada no aparelho, com jogos migrados |
| Botões de volume | P | "Volume NN%" aparece e o som muda, na interface e no jogo |
| Reparo automático do cartão no boot | P | Partição "suja" é verificada com `fsck.vfat`/`fsck.exfat` antes de montar |
| Bateria crítica | P | Abaixo de 3%: salva o jogo, avisa e desliga |
| Validar no aparelho: Wi-Fi, SSH, TF2, Mega Drive, PS1, brilho | M | Tudo funcionando, problemas corrigidos |
| Fone de ouvido | M | Ao conectar o fone, o alto-falante desliga |
| Atalhos padronizados (MENU + X, MENU + L2/R2, MENU + R1) | P | Iguais aos do muOS, documentados |

### Fase 1: Experiência essencial
O mínimo para o Brisa ser o sistema do dia a dia.

| Item | Esforço | Detalhe |
|---|---|---|
| **Quick Resume completo** | M | Ao ligar, volta direto ao último jogo no estado salvo; desliga ao segurar POWER. Opção "começar na lista" |
| **Ociosidade em dois estágios** | M | Apagar a tela após X min, desligar salvando após Y min; nunca enquanto carrega |
| **Wi-Fi pela interface** | M | Lista de redes e teclado na tela |
| Favoritos e busca | M | Busca por letras com teclado na tela; favoritos como pseudo-sistema |
| Ordenação e pastas | P | Pastas dentro do sistema; ordenar por nome, recentes e mais jogados |
| **Atualização sem regravar** | G | Pacote `Brisa/update/brisa-*.tar.xz` com checksum, aplicado no boot, com cópia do sistema anterior para voltar atrás |
| Capturas de tela | P | Atalho padronizado; salvas em `Brisa/screenshots` |
| Idioma inglês | M | Textos da interface em arquivo de tradução |
| Bateria mais precisa | P | Porcentagem estável e estimativa de tempo restante |

### Fase 2: Biblioteca bonita
| Item | Esforço | Detalhe |
|---|---|---|
| **Capas dos jogos** | G | Baixar do **libretro-thumbnails** (sem conta) pelo nome do arquivo; download pelo aparelho via Wi-Fi ou por script no PC; miniaturas redimensionadas e cacheadas para manter a interface rápida |
| Tela do jogo | M | Capa, sistema, tempo jogado, último acesso, opções |
| **Tempo de jogo** | M | Registro por jogo; "mais jogados" e totais |
| Temas leves | M | `theme.conf` com cores, fonte e papel de parede; 3 a 4 temas inclusos |
| Ajustes por jogo | M | Core, governor e "continuar de onde parou" por jogo, herdando do sistema |
| Filtros de cor / acessibilidade | P | Fonte maior, alto contraste, predefinições para daltonismo |

### Fase 3: Mais sistemas
Primeiro os leves, que rodam com folga no H700; depois os pesados, sistema por sistema e **com medição**.

| Grupo | Cores candidatos | Esforço |
|---|---|---|
| Leves | WonderSwan (beetle-wswan), Atari 2600 (stella2014), Atari 7800 (prosystem), Lynx (handy), Neo Geo Pocket (beetle-ngp), PC Engine (beetle-pce-fast), Virtual Boy, Pokémon Mini | M |
| Arcade | FinalBurn Neo (CPS1/2/3, Neo Geo) e MAME 2003-Plus, com BIOS e romsets documentados | M |
| Diversos | ScummVM, PICO-8 (fake-08), EasyRPG, OpenBOR, DOS (DOSBox Pure) | M |
| Pesados | PSP (PPSSPP), N64 (Mupen64Plus-Next), Dreamcast (Flycast), NDS (melonDS DS ou DraStic), Saturn (Yabasanshiro) | G |
| PortMaster | Avaliar as dependências (runtime, SDL2, bibliotecas) e o espaço necessário | G |

Cada sistema novo entra com um perfil medido (governor, run-ahead, latência de áudio) e uma linha na tabela de desempenho.

### Fase 4: Rede e nuvem
| Item | Esforço | Detalhe |
|---|---|---|
| **Painel Brisa (web)** | G | Servidor leve no aparelho; enviar ROMs, baixar saves e capturas, Wi-Fi, logs; QR code na tela; ativado por sessão, com senha |
| **Sincronização de saves** | M | Syncthing ou, mais leve, envio para uma pasta do PC ao sair do jogo |
| RetroAchievements | P | Login na interface; o RetroArch já suporta |
| Relógio pela rede e fuso na interface | P | NTP já existe; falta a interface |
| Integração com RomM | M | Baixar jogos de um servidor RomM próprio |
| Netplay | M | Jogar online pelo RetroArch |

### Fase 5: Hardware e plataforma
| Item | Esforço | Detalhe |
|---|---|---|
| HDMI | M | Detectar o cabo, trocar a saída de vídeo e o áudio para `hw:2,0` |
| Bluetooth | G | Controles e fones; o chip RTL8821CS tem Bluetooth; requer BlueZ |
| Outros aparelhos H700 | G | RG35XX Plus/2024/SP (dobradiça), RG40XX H/V, RG CubeXX (720×720: a interface já escala) e RG28XX (tela girada) |
| Kernel mainline (experimental) | G | Ramo separado, aproveitando o trabalho do ROCKNIX; GPU via Panfrost |

---

## 6. Engenharia e qualidade (contínuo)

| Item | Detalhe |
|---|---|
| **CI no GitHub Actions** | Compilar a interface, rodar `make test` e o build do Buildroot sem GPU a cada push |
| **Orçamento de boot** | Medir o tempo do POWER até a interface e até o jogo (bootchart); falhar o CI se piorar |
| **Bancada de bateria e desempenho** | Script no modo dev que roda cada sistema por X minutos e registra FPS, temperatura e consumo |
| Testes no aparelho | Rotinas no modo dev que validam som, vídeo, Wi-Fi e cartão a cada imagem nova |
| Documentação | Guia de contribuição, arquitetura e "como adicionar um sistema" |
| Versões | Numeração e notas de versão (changelog) a cada imagem |

---

## 7. Próximas entregas sugeridas

| Ordem | Entrega | Por quê |
|---|---|---|
| 1 | Fechar a Fase 0 (volume, expansão, cartão à prova de falhas, validações) | Base confiável antes de qualquer recurso novo |
| 2 | Quick Resume + ociosidade | É o "instant-on", o principal diferencial |
| 3 | Wi-Fi pela interface + Painel Brisa (versão inicial: enviar ROMs e saves) | Elimina a necessidade de tirar o cartão do aparelho |
| 4 | Sistemas leves + arcade | Maior ganho de conteúdo pelo menor esforço; cobre a sua coleção (Atari, CPS, Neo Geo, WonderSwan) |
| 5 | Capas + favoritos + busca | A biblioteca fica com cara de produto pronto |
| 6 | Atualização sem regravar | Necessária antes de convidar mais usuários |

---

## 8. Riscos e decisões em aberto

| Tema | Risco / decisão |
|---|---|
| **Distribuição de imagens** | As imagens contêm bootloader, kernel e libmali proprietários da Anbernic. Hoje o Brisa gera a imagem a partir do firmware do próprio usuário. **Decidir** se continuamos assim ou se publicamos imagens prontas, como fazem muOS e KNULLI |
| Suspender | O KNULLI documenta que suspender drena a bateria no H700 sem solução por software. **Proposta:** Quick Resume como padrão; suspensão só como opção experimental |
| Escopo | Cada recurso novo pesa no tamanho e no tempo de boot. **Regra:** todo recurso precisa caber no orçamento de boot e de memória |
| Kernel 4.9 | Limita ferramentas novas (já impediu o rsync). Reavaliar o mainline quando a Fase 3 estiver pronta |
| PortMaster | Traz muitos jogos, mas também dependências pesadas. Avaliar antes de prometer |
| Emuladores fechados | DraStic (NDS) é fechado; preferir alternativas abertas quando o desempenho permitir |
| Atalhos | Manter os atalhos da Anbernic (segurar START) ou adotar os do muOS (MENU + X) |

---

## 9. Fontes

- muOS / MustardOS: [site oficial](https://muos.dev), [Power Settings](https://muos.dev/tour/modules/muxpower), [Governor](https://muos.dev/tour/modules/muxgov), [Web Services](https://muos.dev/tour/modules/muxwebserv), [Archive Manager](https://muos.dev/tour/modules/muxarchive), [versões](https://muos.dev/release)
- Notícia da versão 2601 "Jacaranda": [retrohandhelds.gg](https://retrohandhelds.gg/mustardos-jacaranda-update-brings-color-controls-and-smarter-ui-features/)
- Guia de terceiros (tempo de boot ~8 s, aparelhos suportados): [Held Games](https://heldgames.com/guides/muos-setup-guide), [retrohandhelds.gg](https://retrohandhelds.gg/muos-setup-guide/)
- KNULLI: [site oficial](https://knulli.org), [Power Management](https://knulli.org/configure/power-management/), [Quick Resume](https://knulli.org/configure/quick-resume/), [Game Settings](https://knulli.org/play/game-settings/), [Scraping](https://knulli.org/play/scraping/), [FAQ](https://knulli.org/faq/knulli/)
- Integração RomM com o muOS: [documentação do RomM](https://docs.romm.app/3.8.3/Integrations/muOS-app/)
