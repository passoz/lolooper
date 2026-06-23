# 🥁 Lolooper

**Live performance looper for Brazilian music — MIDI-controllable, PWA-editable, runs everywhere.**

Lolooper é um sequenciador de padrões rítmicos brasileiros (samba, pagode, ijexá, frevo, maracatu...) feito para **performance ao vivo**. Você edita padrões num PWA, controla tudo via MIDI (controlador físico ou IA), e o som sai onde você quiser:

- **Plugin VST3** no seu DAW junto com voz e instrumento
- **App standalone** no laptop (Windows/Linux/Mac)
- **App standalone** no celular Android (só ele + controlador MIDI USB + mesa)
- **App standalone** no iPhone/iPad (só ele + controlador MIDI Bluetooth + caixa)

---

## Arquitetura

```
┌─────────────────────────────────────────┐
│  PWA (Vue 3 + Pinia + Vite)             │
│  Abre no Chrome, instala como app       │
│  Funciona em: Desktop, Tablet, Celular  │
│                                         │
│  ┌───────────────────────────────────┐  │
│  │  Pattern Editor (grid 14×16)      │  │
│  │  Song Editor (seções em sequência)│  │
│  │  Setlist Manager                  │  │
│  │  Transport (play/stop/BPM/swing)  │  │
│  │  Track Mixer (mute/solo/vol/pan)  │  │
│  │  IA Engine (Web Worker opcional)  │  │
│  └───────────────────────────────────┘  │
│                                         │
│  Persiste em IndexedDB (offline)        │
│  WebMIDI API → porta MIDI virtual       │
└──────────────────┬──────────────────────┘
                   │ MIDI (CC/Note/SysEx)
        ┌──────────┼──────────┐
        ▼          ▼          ▼
┌──────────┐ ┌──────────┐ ┌──────────────┐
│ DESKTOP  │ │ ANDROID  │ │ iOS          │
│          │ │          │ │              │
│ DAW+VST3 │ │ Standalone│ │ Standalone   │
│ ou Stand-│ │ app .apk │ │ app .ipa     │
│ alone    │ │ + MIDI   │ │ + MIDI       │
│          │ │ USB      │ │ Bluetooth    │
└──────────┘ └──────────┘ └──────────────┘
  mesmo código C++ (JUCE) — targets diferentes
```

**Duas codebases. Todos os formatos. Comunicação 100% MIDI.**

---

## Por que essa arquitetura?

| Decisão | Motivo |
|---|---|
| **JUCE (C++)** | Áudio em tempo real exige C++. JUCE compila o MESMO código pra VST3, Standalone Desktop, Android e iOS. |
| **PWA Vue (navegador)** | Editor confortável, IndexedDB offline, WebMIDI nativo. Instalável como app em qualquer dispositivo. |
| **MIDI como protocolo** | Latência <1ms, nativo do DAW, mesmo canal pra IA e controlador físico. Zero código de rede. |
| **Sem backend** | Desnecessário. PWA persiste no navegador. MIDI conecta direto. Menos peças = menos falhas. |
| **Multi-target** | Mesmo C++ = VST3 + Desktop + Android + iOS. Leva o setup que quiser pro palco. |

## Cenários de uso

| Cenário | Dispositivo | Formato | Comunicação | Equipamento extra |
|---|---|---|---|---|
| **Show completo** | Laptop | VST3 no DAW | PWA → WebMIDI → DAW → Plugin | Voz + instrumento + controlador MIDI |
| **Show compacto** | Celular Android | Standalone .apk | MIDI Bluetooth/USB direto no app | Controlador MIDI + mesa de som |
| **Roda de samba** | iPad | Standalone | MIDI Bluetooth direto no app | Controlador MIDI + caixa portátil |
| **Ensaio / edição** | Qualquer | PWA no navegador | N/A (só edição offline) | Só o dispositivo |
| **Estúdio / gravação** | Desktop | VST3 no DAW | PWA → WebMIDI → DAW → Plugin | Gravação multipista, automação, FX chain |

**Importante:** Nos cenários standalone mobile (Android/iOS), o PWA funciona como **editor offline** — você edita patterns e músicas, exporta JSON, e o app standalone importa. O controle em tempo real no palco é feito via MIDI Bluetooth/USB direto no app, não pelo PWA.

---

## Stack técnica

| Camada | Tecnologia |
|---|---|
| **Motor de áudio** | JUCE 8+ (C++17), VST3 + Standalone (Desktop, Android, iOS) |
| **Áudio Android** | Oboe (nativo, baixa latência) |
| **Áudio iOS** | CoreAudio (nativo) |
| **Frontend** | Vue 3 (Composition API, `<script setup>`), TypeScript |
| **Estado** | Pinia (stores reativas) |
| **Roteamento** | Vue Router (editor / songs / setlist / settings) |
| **Estilo** | Tailwind CSS + Glassmorphism (`backdrop-blur`, `bg-white/10`) |
| **Build** | Vite + `vite-plugin-pwa` |
| **MIDI (PWA)** | WebMIDI API (nativo Chrome/Edge) |
| **MIDI (Android)** | Android MIDI API (USB + Bluetooth) |
| **MIDI (iOS)** | CoreMIDI (USB + Bluetooth) |
| **Persistência** | IndexedDB via `idb-keyval` |
| **IA** | Web Worker (thread separada, opcional) |
| **Drag & drop** | `@vueuse/core` |

---

## Funcionalidades

### Core
- **14 instrumentos** — surdo 1ª/2ª/3ª, caixa, repique, tamborim, pandeiro, cuíca, agogô, reco-reco, tantã, cavaquinho, violão 7 cordas, banjo
- **Grid de 16 semicolcheias** — edição com 4 velocidades (· ○ ◉ ●)
- **Vários estilos rítmicos** — samba, pagode, partido alto, samba-reggae, ijexá, frevo, maracatu
- **Edição por gravação MIDI** — toca no controlador, plugin grava quantizado na grid
- **Song mode** — sequência de seções (intro → verso → refrão → virada), cada uma com pattern + número de compassos
- **Multi-sample por track** — surdo aberto, abafado, com baqueta; pandeiro grave/agudo/platinela
- **Swing e humanize** — swing ajustável (0-100%), humanize (variação aleatória de timing)

### Controle
- **MIDI físico** — pads, knobs, faders de qualquer controlador
- **MIDI da IA** — Web Worker envia CC/Note pela mesma porta virtual
- **PWA touch** — interface amigável pra tablet/touchscreen

### Performance
- **Setlists** — organize músicas por show, com BPM, tom e anotações
- **Exportação** — patterns em JSON, plugin carrega ao abrir projeto do DAW
- **PWA offline** — tudo funciona sem internet
- **Instalável** — ícone no desktop, janela própria, sem barra de navegador

---

## Estrutura do repositório

```
lolooper/
├── plugin/                    ← JUCE (C++)
│   ├── CMakeLists.txt
│   └── Source/
│       ├── PluginProcessor.cpp/h
│       ├── PluginEditor.cpp/h
│       ├── Sequencer.cpp/h
│       ├── Mixer.cpp/h
│       └── SampleLib.cpp/h
│
├── frontend/                  ← Vue 3 + PWA
│   ├── index.html
│   ├── package.json
│   ├── vite.config.ts
│   ├── public/
│   │   ├── manifest.json
│   │   ├── sw.js
│   │   └── icons/
│   └── src/
│       ├── App.vue
│       ├── main.ts
│       ├── components/
│       │   ├── PatternGrid.vue
│       │   ├── Transport.vue
│       │   ├── TrackList.vue
│       │   ├── SongEditor.vue
│       │   └── SetlistManager.vue
│       ├── stores/
│       │   ├── patterns.ts
│       │   ├── songs.ts
│       │   ├── setlist.ts
│       │   └── midi.ts
│       ├── db/
│       │   └── database.ts
│       ├── worker/
│       │   └── ia-worker.ts
│       └── composables/
│           ├── useWebMIDI.ts
│           └── usePattern.ts
│
├── docs/
│   ├── patterns.md            ← Referência dos padrões rítmicos
│   ├── midi-mapping.md        ← Tabela de CC/Note mapping
│   └── song-format.md         ← Formato YAML de músicas
│
├── README.md                  ← Este arquivo
├── AGENTS.md                  ← Instruções para agentes de IA
├── SPEC.md                    ← Especificação técnica
└── TODO.md                    ← Planejamento e tarefas
```

---

## Como usar (visão geral)

### Em casa — preparando o show
1. Abra o PWA (atalho no desktop)
2. Edite patterns na grid 14×16 ou grave via MIDI
3. Organize músicas com seções (intro, verso, refrão)
4. Monte a setlist do show
5. Exporte patterns → cole na pasta do projeto do Bitwig

### No palco — tocando
1. Abra o Bitwig com o projeto do show
2. Abra o PWA
3. Transport, mute/solo, scenes: tudo pelo PWA ou controlador físico
4. IA opcional: ative o Web Worker para assistir/sugerir

---

## Licença

MIT

## Autor

Fernando Passos
