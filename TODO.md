# TODO.md — Planejamento e Tarefas

## Visão geral do progresso

| Fase | Status | Previsão |
|---|---|---|
| **Fase 1: Fundação** | 🔴 Não iniciado | 2 semanas |
| **Fase 2: Plugin — áudio** | 🔴 Não iniciado | 3 semanas |
| **Fase 3: PWA — editor e controle** | 🔴 Não iniciado | 3 semanas |
| **Fase 4: Song mode e setlists** | 🔴 Não iniciado | 2 semanas |
| **Fase 5: IA e polimento** | 🔴 Não iniciado | 2 semanas |
| **Fase 6: Mobile — Android e iOS** | 🔴 Não iniciado | 2 semanas |

---

## Fase 1: Fundação (semanas 1-2)

### 1.1 Estrutura do repositório

- [ ] **1.1.1** Criar `plugin/` com estrutura JUCE mínima (Projucer ou CMake manual)
- [ ] **1.1.2** Criar `frontend/` com `npm create vite@latest` (Vue 3 + TypeScript)
- [ ] **1.1.3** Configurar Tailwind CSS com glassmorphism components
- [ ] **1.1.4** Configurar `vite-plugin-pwa` com manifest e service worker
- [ ] **1.1.5** Configurar `vue-i18n` com idioma português (UI)
- [ ] **1.1.6** Configurar Pinia + Vue Router
- [ ] **1.1.7** Configurar `idb-keyval` para persistência IndexedDB
- [ ] **1.1.8** Criar `.gitignore` para `plugin/Build/`, `frontend/dist/`, `node_modules/`
- [ ] **1.1.9** Configurar ESLint + Prettier (frontend)
- [ ] **1.1.10** Configurar Catch2 (plugin, testes unitários)

### 1.2 Dados padrão

- [ ] **1.2.1** Criar `data/patterns/samba.json` com os 14 instrumentos × 16 steps
- [ ] **1.2.2** Criar `data/patterns/pagode.json`
- [ ] **1.2.3** Criar `data/patterns/partido_alto.json`
- [ ] **1.2.4** Criar `data/patterns/samba_reggae.json`
- [ ] **1.2.5** Criar `data/patterns/ijexa.json`
- [ ] **1.2.6** Criar `data/patterns/frevo.json`
- [ ] **1.2.7** Criar `data/patterns/maracatu.json`
- [ ] **1.2.8** Criar `data/patterns/intro.json` (apenas surdo marcando)
- [ ] **1.2.9** Criar `data/patterns/virada.json` (fill/break tutti)
- [ ] **1.2.10** Função `loadDefaults()` que carrega todos os padrões built-in

---

## Fase 2: Plugin — áudio (semanas 3-5)

### 2.1 Estrutura base do plugin

- [ ] **2.1.1** `PluginProcessor.h/cpp` — AudioProcessor vazio que compila e carrega no DAW
- [ ] **2.1.2** `PluginEditor.h/cpp` — Editor vazio (placeholder)
- [ ] **2.1.3** Criar parâmetros VST3: BPM, swing, humanize, pattern style, mute/solo/volume/pan por track
- [ ] **2.1.4** Implementar `getStateInformation()` / `setStateInformation()` (serialização)

### 2.2 Sequenciador

- [ ] **2.2.1** `Sequencer.h/cpp` — classe do sequenciador
- [ ] **2.2.2** Implementar `advance(samplesInBlock)` — avança passo baseado em BPM
- [ ] **2.2.3** Implementar cálculo de samples por step: `60.0 / bpm / 4.0 * sampleRate`
- [ ] **2.2.4** Implementar swing: atrasar passos ímpares por `swing * stepDuration`
- [ ] **2.2.5** Implementar humanize: offset aleatório por hit (± `humanize * stepDuration`)
- [ ] **2.2.6** Expor estado: `getCurrentStep()`, `getCurrentBeat()`, `getCurrentBar()`
- [ ] **2.2.7** Transport: `play()`, `stop()`, `pause()`, `isPlaying()`
- [ ] **2.2.8** Testes unitários: precisão do BPM, swing, humanize

### 2.3 Sample Library

- [ ] **2.3.1** `SampleLib.h/cpp` — carregamento de samples
- [ ] **2.3.2** Suporte a WAV e FLAC via JUCE `AudioFormatReader`
- [ ] **2.3.3** Cache em memória (`HashMap<String, AudioBuffer<float>>`)
- [ ] **2.3.4** `loadSample(name, path)` — carrega e armazena
- [ ] **2.3.5** `getSample(name)` — retorna buffer ou nullptr
- [ ] **2.3.6** Suporte a multi-sample por track (sample slots: open, muted, rim...)
- [ ] **2.3.7** Testes unitários: carregamento, fallback, sample rate diferente

### 2.4 Mixer

- [ ] **2.4.1** `Mixer.h/cpp` — classe do mixer de áudio
- [ ] **2.4.2** `renderStep(step, nSamples, outBuffer)` — loop principal de renderização
- [ ] **2.4.3** Para cada track: `velocity = pattern[track][step] * accent`
- [ ] **2.4.4** Se velocity > 0: copiar sample × velocity × volume × pan
- [ ] **2.4.5** Pan: mono → stereo com ganho L/R `(1-pan, pan)`
- [ ] **2.4.6** Soft clipping: `juce::FloatVectorOperations::clip`
- [ ] **2.4.7** Respeitar mute, solo, active flags
- [ ] **2.4.8** Testes unitários: mixagem correta, pan, clipping

### 2.5 MIDI Input

- [ ] **2.5.1** Processar CC messages → atualizar parâmetros
- [ ] **2.5.2** Processar Note On/Off → toggle mute, play/stop/rec, scenes
- [ ] **2.5.3** Processar SysEx → carregar pattern, song structure
- [ ] **2.5.4** Mapeamento CC completo (volume 20-33, pan 34-47, accent 48-61)
- [ ] **2.5.5** Mapeamento Note completo (mute 36-49, transport 114-122)
- [ ] **2.5.6** SysEx protocol: request patterns (0x01), response (0x02), push (0x03)
- [ ] **2.5.7** Testes unitários: parsing de CC, Note, SysEx

### 2.6 Pattern management no plugin

- [ ] **2.6.1** Estrutura `Pattern` com grid 14×16 float
- [ ] **2.6.2** `Pattern::fromJson(String)` / `Pattern::toJson()`
- [ ] **2.6.3** Carregar `default_patterns.json` ao iniciar
- [ ] **2.6.4** Responder SysEx request com padrões atuais
- [ ] **2.6.5** Receber push de pattern via SysEx e substituir em tempo real

### 2.7 Gravação MIDI (REC mode)

- [ ] **2.7.1** Ativar/desativar modo REC via Note 117
- [ ] **2.7.2** Capturar Note On durante REC
- [ ] **2.7.3** Quantizar na grid: `note → track`, `step atual → posição`
- [ ] **2.7.4** Sobrescrever ou merge com pattern existente (configurável)
- [ ] **2.7.5** Enviar pattern resultante via SysEx ao parar REC

### 2.8 Editor do plugin (mínimo)

- [ ] **2.8.1** Transport: botões play/stop/rec
- [ ] **2.8.2** Display de BPM editável
- [ ] **2.8.3** Mini track list: nome + mute/solo + indicador de atividade
- [ ] **2.8.4** Indicador de step atual (piscando na grid pequena)
- [ ] **2.8.5** Indicador de MIDI activity (led piscando)

---

## Fase 3: PWA — editor e controle (semanas 6-8)

### 3.1 Estrutura Vue + Stores

- [ ] **3.1.1** `App.vue` com `<router-view>` e navegação
- [ ] **3.1.2** `patternsStore.ts` — Pinia store para patterns
- [ ] **3.1.3** `songsStore.ts` — Pinia store para músicas/seções
- [ ] **3.1.4** `setlistStore.ts` — Pinia store para setlists
- [ ] **3.1.5** `midiStore.ts` — Pinia store para conexão WebMIDI
- [ ] **3.1.6** Persistência automática: toda store salva no IndexedDB via `idb-keyval`
- [ ] **3.1.7** Plugin `pinia-plugin-persistedstate` para sincronização automática

### 3.2 WebMIDI — camada de comunicação

- [ ] **3.2.1** `composables/useWebMIDI.ts` — conexão e envio
- [ ] **3.2.2** `connect()` — `navigator.requestMIDIAccess()`, listar portas
- [ ] **3.2.3** `sendCC(cc, value)` — enviar Control Change
- [ ] **3.2.4** `sendNote(note, velocity)` — enviar Note On + Note Off
- [ ] **3.2.5** `sendSysEx(data)` — enviar dados binários
- [ ] **3.2.6** `onMessage(callback)` — receber MIDI input (feedback do plugin)
- [ ] **3.2.7** Indicador visual de conexão MIDI (verde = conectado, vermelho = offline)
- [ ] **3.2.8** Detecção de porta virtual "Lolooper" na lista

### 3.3 Pattern Editor (Grid)

- [ ] **3.3.1** `PatternGrid.vue` — grid 14×16 células
- [ ] **3.3.2** Cada célula: 4 estados visuais (· ○ ◉ ●) correspondendo a 0, 0.4, 0.7, 1.0
- [ ] **3.3.3** Clique: cicla entre os 4 estados
- [ ] **3.3.4** Drag: seleciona múltiplas células e aplica mesmo valor
- [ ] **3.3.5** Labels: nomes dos instrumentos à esquerda de cada linha
- [ ] **3.3.6** Cabeçalho: números dos steps (1-16) no topo
- [ ] **3.3.7** Destaque visual do beat (linha vertical a cada 4 steps)
- [ ] **3.3.8** Atalhos de teclado: 1-4 para velocidades, Ctrl+Z/Y undo/redo
- [ ] **3.3.9** Copiar/colar track: botão direito → menu contextual
- [ ] **3.3.10** Seletor de estilo: dropdown para trocar entre patterns (samba, pagode...)
- [ ] **3.3.11** Botão "Novo estilo": duplica pattern atual com novo nome
- [ ] **3.3.12** Botão "Exportar JSON": baixa arquivo
- [ ] **3.3.13** Botão "Importar JSON": upload de arquivo
- [ ] **3.3.14** Indicador de modificações não salvas

### 3.4 Track List / Mixer

- [ ] **3.4.1** `TrackList.vue` — lista vertical das 14 tracks
- [ ] **3.4.2** Nome do instrumento com ícone (surdo, pandeiro, etc.)
- [ ] **3.4.3** Botão Mute (toggle) — envia CC MIDI
- [ ] **3.4.4** Botão Solo (toggle) — envia CC MIDI
- [ ] **3.4.5** Slider de volume — envia CC MIDI em tempo real
- [ ] **3.4.6** Slider de pan — envia CC MIDI
- [ ] **3.4.7** Indicador de atividade (pisca no beat daquele instrumento)
- [ ] **3.4.8** Indicador "sample loaded" (verde = sim, cinza = não)
- [ ] **3.4.9** Indicador de accent atual

### 3.5 Transport

- [ ] **3.5.1** `Transport.vue` — barra de transporte
- [ ] **3.5.2** Botão Play/Pause — envia Note MIDI
- [ ] **3.5.3** Botão Stop — envia Note MIDI
- [ ] **3.5.4** Botão Record — envia Note MIDI, muda cor quando ativo
- [ ] **3.5.5** Display de BPM com +/- e input numérico
- [ ] **3.5.6** Slider de Swing (0-100%)
- [ ] **3.5.7** Slider de Humanize (0-50ms)
- [ ] **3.5.8** Indicador de compasso atual (bar.beat.step)
- [ ] **3.5.9** Seletor de pattern style (dropdown, muda todos os tracks)

### 3.6 Roteamento e navegação

- [ ] **3.6.1** Vue Router: `/editor` (padrão), `/songs`, `/setlists`, `/settings`
- [ ] **3.6.2** Barra de navegação com ícones
- [ ] **3.6.3** Glassmorphism consistente em todas as páginas
- [ ] **3.6.4** Responsivo: funciona bem em tablet (1024px+) e desktop
- [ ] **3.6.5** Modo escuro por padrão (fundo escuro com glass)

---

## Fase 4: Song mode e setlists (semanas 9-10)

### 4.1 Song Editor

- [ ] **4.1.1** `SongEditorPage.vue` — página de edição de música
- [ ] **4.1.2** Campos: nome da música, BPM, tom, estilo
- [ ] **4.1.3** Lista de seções: ordem, nome, pattern, número de compassos
- [ ] **4.1.4** Drag-and-drop para reordenar seções (`@vueuse/core` `useDraggable`)
- [ ] **4.1.5** Editor de ações por seção (ex: "mute cavaquinho no compasso 0")
- [ ] **4.1.6** Pré-visualização: timeline vertical da música
- [ ] **4.1.7** Botão "Testar": envia estrutura da música pro plugin via SysEx
- [ ] **4.1.8** Exportar música como JSON/YAML
- [ ] **4.1.9** Importar música de JSON/YAML

### 4.2 Setlist Manager

- [ ] **4.2.1** `SetlistPage.vue` — página de setlists
- [ ] **4.2.2** Criar nova setlist (nome, data do show)
- [ ] **4.2.3** Adicionar músicas do repertório
- [ ] **4.2.4** Drag-and-drop para reordenar
- [ ] **4.2.5** Visualização: ordem das músicas, BPM de cada uma, duração estimada
- [ ] **4.2.6** Modo "Show": tela cheia, foco na música atual + próxima
- [ ] **4.2.7** Transições: aviso visual "próxima música em 4 compassos"
- [ ] **4.2.8** Histórico: registrar setlists passadas

### 4.3 Comunicação Song Mode (PWA → Plugin)

- [ ] **4.3.1** SysEx 0x04: enviar estrutura completa da música
- [ ] **4.3.2** Plugin implementa contagem de compassos por seção
- [ ] **4.3.3** Plugin avança seção automaticamente
- [ ] **4.3.4** Plugin envia feedback (seção atual, compasso) via SysEx
- [ ] **4.3.5** PWA mostra progresso: bar.beat + nome da seção
- [ ] **4.3.6** PWA pode pular seção (Note 119/120)

---

## Fase 5: IA e polimento (semanas 11-12)

### 5.1 IA Engine (Web Worker)

- [ ] **5.1.1** `worker/ia-worker.ts` — estrutura base do worker
- [ ] **5.1.2** Interface de estado: bar, beat, section, energy
- [ ] **5.1.3** Interface de ações: mute, unmute, volume, pattern, trigger, nextSection
- [ ] **5.1.4** Modo "Assiste" — monitora, não age
- [ ] **5.1.5** Modo "Auxilia" — sugere ações (precisa confirmação)
- [ ] **5.1.6** Modo "Toma conta" — age automaticamente
- [ ] **5.1.7** Estratégia básica: reconhecer seções e aplicar ações pré-definidas
- [ ] **5.1.8** Estratégia de energia: detectar refrão e aumentar accent
- [ ] **5.1.9** Estratégia de variação: alternar patterns similares pra evitar repetição

### 5.2 Painel da IA no PWA

- [ ] **5.2.1** `IAPanel.vue` — componente na página de settings
- [ ] **5.2.2** Toggle: liga/desliga IA
- [ ] **5.2.3** Seletor de modo: Assiste / Auxilia / Toma conta
- [ ] **5.2.4** Indicador de confiança (barra horizontal)
- [ ] **5.2.5** Log de ações da IA: "Mutou cavaquinho (verso, compasso 2)"
- [ ] **5.2.6** Botão "Override": ignorar próxima ação da IA

### 5.3 Testes e estabilidade

- [ ] **5.3.1** Teste de longa duração: 4 horas contínuas sem crash
- [ ] **5.3.2** Teste de reconexão MIDI: desconectar e reconectar porta virtual
- [ ] **5.3.3** Teste offline: PWA funcionando sem internet
- [ ] **5.3.4** Teste de carga: 14 tracks tocando simultaneamente
- [ ] **5.3.5** Teste de BPM extremo: 20 BPM e 300 BPM
- [ ] **5.3.6** Teste de swing extremo: 0% e 100%
- [ ] **5.3.7** Teste cross-platform: plugin no Linux (Bitwig) e Windows (Ableton)
- [ ] **5.3.8** Build standalone do plugin e teste sem DAW

### 5.4 UI Polimento

- [ ] **5.4.1** Glassmorphism refinado em todos os componentes
- [ ] **5.4.2** Animações suaves (transições de página, hover, feedback tátil)
- [ ] **5.4.3** Cores por instrumento (surdo = azul escuro, caixa = laranja, etc.)
- [ ] **5.4.4** Tooltips em todos os botões e células
- [ ] **5.4.5** Atalhos de teclado documentados (pressionar `?` mostra cheat sheet)
- [ ] **5.4.6** Loading states e skeletons
- [ ] **5.4.7** Error boundaries (MIDI disconnect, IndexedDB quota)
- [ ] **5.4.8** Suporte i18n completo (português + inglês)

### 5.5 Documentação

- [ ] **5.5.1** `docs/patterns.md` — guia de edição de patterns
- [ ] **5.5.2** `docs/midi-mapping.md` — tabela completa de CC/Note
- [ ] **5.5.3** `docs/song-format.md` — formato YAML de músicas
- [ ] **5.5.4** `docs/getting-started.md` — tutorial passo a passo
- [ ] **5.5.5** `docs/ia-guide.md` — como a IA funciona e como treiná-la
- [ ] **5.5.6** `docs/contributing.md` — como contribuir
- [ ] **5.5.7** `docs/troubleshooting.md` — problemas comuns

### 5.6 Release — Desktop

- [ ] **5.6.1** Build de release do plugin (VST3 Linux, VST3 Windows)
- [ ] **5.6.2** Build standalone (Linux, Windows)
- [ ] **5.6.3** Deploy do PWA (GitHub Pages ou Netlify)
- [ ] **5.6.4** Criar release no GitHub com assets desktop
- [ ] **5.6.5** CHANGELOG.md
- [ ] **5.6.6** Gravar demo em vídeo (3 minutos)

---

## Fase 6: Mobile — Android e iOS (semanas 13-14)

Mesmo código C++ do plugin. Apenas novos targets de build e adaptações de UI.

### 6.1 Build Android

- [ ] **6.1.1** Configurar Android SDK + NDK no ambiente de build
- [ ] **6.1.2** Configurar CMake target Android standalone (JUCE `FORMATS Standalone`)
- [ ] **6.1.3** Áudio: JUCE usa Oboe automaticamente (baixa latência nativa)
- [ ] **6.1.4** MIDI: JUCE usa Android MIDI API (USB + Bluetooth LE)
- [ ] **6.1.5** Testar com controlador MIDI USB
- [ ] **6.1.6** Testar com controlador MIDI Bluetooth
- [ ] **6.1.7** Gerar .apk / .aab
- [ ] **6.1.8** Teste de latência (alvo: <30ms com Oboe)
- [ ] **6.1.9** Teste de estabilidade (1h contínua sem crash)

### 6.2 Build iOS

- [ ] **6.2.1** Configurar Xcode + JUCE para iOS
- [ ] **6.2.2** Configurar CMake target iOS standalone
- [ ] **6.2.3** Áudio: JUCE usa CoreAudio automaticamente
- [ ] **6.2.4** MIDI: JUCE usa CoreMIDI (USB via Camera Kit + Bluetooth)
- [ ] **6.2.5** Testar com controlador MIDI Bluetooth
- [ ] **6.2.6** Gerar .ipa (TestFlight ou sideload)
- [ ] **6.2.7** Teste de latência (alvo: <10ms com CoreAudio)

### 6.3 UI Mobile

- [ ] **6.3.1** Adaptar layout da UI JUCE pra touch (botões maiores, gestos)
- [ ] **6.3.2** Testar PWA no Chrome Android como controle (WebMIDI → app standalone)
- [ ] **6.3.3** Testar PWA no Safari iOS como controle visual
- [ ] **6.3.4** Suporte a orientação (portrait + landscape)

### 6.4 Release Mobile

- [ ] **6.4.1** Publicar na Google Play Store (Android)
- [ ] **6.4.2** Publicar na Apple App Store (iOS) — requer Apple Developer account
- [ ] **6.4.3** Atualizar README com links das lojas
