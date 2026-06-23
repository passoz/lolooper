# SPEC.md — Especificação Técnica do Lolooper

## 1. Visão Geral

Lolooper é um looper/sequenciador para performance ao vivo de música brasileira. Consiste em duas peças independentes:

| Peça | Tecnologia | Descrição |
|---|---|---|
| **Plugin** | JUCE 8+ (C++) | VST3/Standalone. Processa áudio, sequenciador, mixer, MIDI input. |
| **PWA** | Vue 3 + TypeScript | Editor de patterns, controle de performance, IA opcional. |

Comunicação entre as peças: **exclusivamente MIDI** via porta virtual (loopMIDI no Windows, `snd-virmidi` no Linux, IAC Driver no Mac).

---

## 2. Plugin (JUCE)

### 2.1 Classes principais

```
LolooperAudioProcessor : juce::AudioProcessor
├── processBlock(AudioBuffer<float>&, MidiBuffer&)
├── Sequencer m_sequencer
├── Mixer m_mixer
├── SampleLib m_samples
├── juce::AudioParameterFloat* (BPM, swing, humanize)
├── juce::AudioParameterChoice* (pattern style)
├── juce::AudioParameterFloat* trackVolume[14]
├── juce::AudioParameterFloat* trackPan[14]
├── juce::AudioParameterBool* trackMute[14]
└── juce::AudioParameterBool* trackSolo[14]

LolooperAudioProcessorEditor : juce::AudioProcessorEditor
├── Transport (play/stop/rec/BPM)
├── Mini track list (mute/solo/volume)
└── Indicador de step atual

Sequencer
├── advance(): calcula passo atual baseado em BPM e bufferSize
├── getCurrentStep(): int (0-15)
├── getCurrentBeat(): int (0-3)
├── getCurrentBar(): int
├── setBpm(float)
├── setSwing(float)
└── setHumanize(float)

Mixer
├── renderStep(int step, int nSamples, AudioBuffer<float>& out)
├── Para cada track ativo:
│   ├── velocity = pattern[track][step] × accent × humanizeOffset
│   ├── sampleData × velocity × volume × pan
│   └── soma no buffer de saída
└── applySoftClip(AudioBuffer<float>&)

SampleLib
├── loadSample(String name, File path) → float*
├── getSample(String name) → AudioBuffer<float>&
└── preloadAll(File directory)

Pattern
├── String name ("samba", "pagode"...)
├── float grid[14][16]     ← 14 instrumentos × 16 steps
└── fromJson(String) / toJson() → String
```

### 2.2 Parâmetros VST3 (automatizáveis)

| Parâmetro | Tipo | Range | Default |
|---|---|---|---|
| BPM | Float | 20-300 | 100 |
| Swing | Float | 0-1 | 0 |
| Humanize | Float | 0-1 | 0 |
| Pattern Style | Choice | samba, pagode, partido_alto, samba_reggae, ijexa, frevo, maracatu, intro, virada | samba |
| Track 1-14 Volume | Float | 0-1 | 0.8 |
| Track 1-14 Pan | Float | 0-1 | 0.5 |
| Track 1-14 Mute | Bool | 0/1 | 0 |
| Track 1-14 Solo | Bool | 0/1 | 0 |
| Track 1-14 Accent | Float | 0-2 | 1.0 |
| Scene Trigger | Int | 0-127 | 0 |

### 2.3 Processamento de áudio (`processBlock`)

```
1. Processa MIDI input:
   - CC: atualiza parâmetros (volume, pan, mute, BPM, swing)
   - Note On: toggle mute, play/stop/rec, scene triggers
   - SysEx: carrega pattern, song structure

2. Se transport parado: silêncio, retorna

3. Avança sequenciador:
   - Calcula samples por step (60/BPM/4 em samples)
   - Determina passo atual, beat, barra
   - Aplica swing (atrasa passos ímpares)

4. Renderiza áudio:
   - Para cada track ativo (não mutado, respeita solo):
     - velocity = pattern[track][step] × accent × humanize
     - Se velocity > 0: copia sample data × velocity × volume
     - Aplica pan (mono → stereo com ganho L/R)
     - Soma no buffer de saída

5. Soft clipping: np.clip(out, -0.99, 0.99)

6. Se song mode ativo:
   - Conta compassos da seção atual
   - Avança seção quando completar compassos
   - Se última seção: loop ou stop (configurável)

7. Se REC ativo:
   - Captura MIDI notes recebidas
   - Quantiza na grid do passo atual
```

---

## 3. PWA (Vue 3 + TypeScript)

### 3.1 Estrutura de componentes

```
App.vue
├── AppHeader.vue (logo, navegação)
├── <router-view>
│   ├── PatternEditorPage.vue
│   │   ├── PatternGrid.vue
│   │   ├── TrackList.vue
│   │   └── Transport.vue
│   ├── SongEditorPage.vue
│   │   ├── SongStructure.vue
│   │   └── SectionEditor.vue
│   ├── SetlistPage.vue
│   │   ├── SetlistList.vue
│   │   └── SetlistEditor.vue
│   └── SettingsPage.vue
│       ├── MIDIConfig.vue
│       └── IAPanel.vue
└── AppFooter.vue (status MIDI, relógio)
```

### 3.2 Stores (Pinia)

```typescript
// patternsStore.ts
interface PatternStore {
  styles: Map<string, Pattern>;        // "samba" → grid 14×16
  activeStyle: string;                 // estilo atual
  editingStyle: string;                // qual está sendo editado
  history: Pattern[][];                // undo stack
  historyIndex: number;

  cycleVelocity(track: number, step: number): void;
  setVelocity(track: number, step: number, vel: number): void;
  copyTrack(from: number, to: number): void;
  pasteTrack(source: string, style: string): void;
  undo(): void;
  redo(): void;
  exportJson(): string;
  importJson(json: string): void;
  loadDefaults(): void;                // Carrega padrões brasileiros built-in
}

// songsStore.ts
interface SongStore {
  songs: Song[];
  currentSong: Song | null;

  addSong(song: Song): void;
  removeSong(id: string): void;
  updateSong(id: string, data: Partial<Song>): void;
}

interface Song {
  id: string;
  name: string;
  bpm: number;
  key: string;                          // "Cm", "F#m"
  style: string;                        // "samba", "ijexa"
  sections: Section[];
}

interface Section {
  name: string;                         // "intro", "verso", "refrão"
  patternStyle: string;                 // qual pattern usar
  bars: number;                         // quantos compassos
  actions: SectionAction[];             // ações no início da seção
}

// setlistStore.ts
interface SetlistStore {
  setlists: Setlist[];
  activeSetlist: Setlist | null;

  createSetlist(name: string): void;
  addSongToSetlist(setlistId: string, songId: string): void;
  reorder(setlistId: string, from: number, to: number): void;
}

interface Setlist {
  id: string;
  name: string;
  date: string;
  songs: string[];                      // IDs das músicas em ordem
}

// midiStore.ts
interface MidiStore {
  outputs: MIDIOutput[];
  inputs: MIDIInput[];
  activeOutput: MIDIOutput | null;
  connected: boolean;

  connect(): Promise<void>;
  sendCC(channel: number, cc: number, value: number): void;
  sendNote(note: number, velocity: number): void;
  sendSysEx(data: number[]): void;
  onMessage(callback: (msg: MIDIMessageEvent) => void): void;
}
```

### 3.3 WebMIDI — conexão e envio

```typescript
// composables/useWebMIDI.ts
export function useWebMIDI() {
  const midi = reactive({ outputs: [], inputs: [], connected: false });

  async function connect() {
    const access = await navigator.requestMIDIAccess();
    midi.outputs = [...access.outputs.values()];
    midi.inputs = [...access.inputs.values()];
    midi.connected = true;
  }

  function sendCC(cc: number, value: number, channel: number = 0) {
    const output = midi.outputs[0];
    if (!output) return;
    output.send([0xB0 | channel, cc, value]);
  }

  function sendNote(note: number, velocity: number = 127) {
    const output = midi.outputs[0];
    if (!output) return;
    output.send([0x90, note, velocity]);
    output.send([0x80, note, 0], window.performance.now() + 50); // Note Off após 50ms
  }

  return { midi, connect, sendCC, sendNote };
}
```

### 3.4 Persistência (IndexedDB)

```typescript
// db/database.ts
import { get, set, del, keys } from 'idb-keyval';

export const db = {
  // Patterns
  async loadPatterns(): Promise<Record<string, Pattern>> { return await get('patterns') || {}; },
  async savePatterns(data: Record<string, Pattern>): Promise<void> { await set('patterns', data); },

  // Songs
  async loadSongs(): Promise<Song[]> { return await get('songs') || []; },
  async saveSongs(data: Song[]): Promise<void> { await set('songs', data); },

  // Setlists
  async loadSetlists(): Promise<Setlist[]> { return await get('setlists') || []; },
  async saveSetlists(data: Setlist[]): Promise<void> { await set('setlists', data); },

  // Settings
  async loadSettings(): Promise<Settings> { return await get('settings') || defaults; },
  async saveSettings(data: Settings): Promise<void> { await set('settings', data); },
};
```

### 3.5 PWA — manifest e service worker

```json
{
  "name": "Lolooper",
  "short_name": "Lolooper",
  "description": "Live performance looper for Brazilian music",
  "start_url": "/",
  "display": "standalone",
  "background_color": "#0a0a0a",
  "theme_color": "#1a1a2e",
  "icons": [
    { "src": "/icons/icon-192.png", "sizes": "192x192", "type": "image/png" },
    { "src": "/icons/icon-512.png", "sizes": "512x512", "type": "image/png" }
  ]
}
```

### 3.6 IA Engine (Web Worker)

```typescript
// worker/ia-worker.ts
// Roda em thread separada. Recebe estado atual, decide ações.

interface IAState {
  bar: number;
  beat: number;
  section: string;
  sectionsRemaining: number;
  energy: number;         // 0-1, calculado pela IA
}

interface IAAction {
  type: 'mute' | 'unmute' | 'volume' | 'pattern' | 'triggerVirada' | 'nextSection';
  track?: string;
  value?: number;
}

// Estratégias da IA:
// - "Assiste" (só monitora, não age)
// - "Auxilia" (sugere, mas não age sem confirmação)
// - "Toma conta" (age automaticamente)

self.onmessage = (e: MessageEvent<IAState>) => {
  const actions = decide(e.data);
  self.postMessage(actions);
};
```

---

## 4. Formatos de arquivo

### 4.1 Pattern (JSON)

```json
{
  "samba": {
    "surdo_1":   [0,0,0,0,1.0,0,0,0,0,0,0,0,1.0,0,0,0],
    "surdo_2":   [1.0,0,0,0,0,0,0,0,0.8,0,0,0,0,0,0,0],
    "surdo_3":   [0,0,0,0.8,0,0,0,0,0,0,0,0,0,0.7,0.6,0.9],
    "caixa":     [0,0,1.0,0,0.6,0,0.9,0,0,0,1.0,0,0.6,0,0.8,0],
    "repique":   [0,0,0.7,0,0,0,0.8,0,0,0,0.6,0,0,0,0.9,0],
    "tamborim":  [0.7,0.4,0.7,0.4,0.7,0.4,0.9,0.5,0.7,0.4,0.7,0.4,0.9,0.5,1.0,0.6],
    "pandeiro":  [0.6,0.3,0.8,0.4,0.5,0.3,0.7,0.9,0.6,0.3,0.8,0.4,0.5,0.2,1.0,0.8],
    "cuica":     [0,0,0,0,0.9,0,0,0,0,0,0.7,0,0,0.6,0,0],
    "agogo":     [0.8,0,0.6,0,0.8,0,1.0,0,0.8,0,0.6,0,0.8,0,0.9,0],
    "reco_reco": [0.5,0.4,0.5,0.4,0.5,0.4,0.5,0.4,0.5,0.4,0.5,0.4,0.5,0.4,0.5,0.4],
    "tantan":    [0,0,1.0,0,0,0.5,0,0,0,0,0.8,0,0,0,0.4,0],
    "cavaquinho":[0.8,0.4,0.8,0.4,0.8,0.4,0.9,0.5,0.8,0.4,0.8,0.4,0.9,0.5,1.0,0.6],
    "violao_7":  [0.9,0,0,0.5,0,0,0.6,0,0.7,0,0,0.4,0,0,0.8,0],
    "banjo":     [0.7,0.3,0.7,0.3,0.8,0.4,0.7,0.3,0.7,0.3,0.7,0.3,0.9,0.4,1.0,0.5]
  }
}
```

### 4.2 Song (YAML)

```yaml
song:
  name: "Samba de Roda"
  bpm: 104
  key: "Dm"
  style: "samba"
  sections:
    - name: intro
      pattern: intro
      bars: 2
    - name: verso
      pattern: samba
      bars: 16
      actions:
        - at_bar: 0
          cmd: mute
          track: cavaquinho
    - name: pre_refrao
      pattern: samba_forte
      bars: 4
      actions:
        - at_bar: 0
          cmd: unmute_all
    - name: refrao
      pattern: samba
      bars: 16
      actions:
        - at_bar: 0
          cmd: set_accent
          track: pandeiro
          value: 1.3
    - name: virada
      pattern: virada
      bars: 1
    - name: fim
      pattern: intro
      bars: 4
      actions:
        - at_bar: 2
          cmd: fade_out
          duration: 2
```

### 4.3 Multi-sample track definition (futuro — pós 1.0)

> ⚠️ **Não implementar na Fase 2.** O modelo inicial usa float grid simples (1 sample por track). Multi-sample será adicionado após o 1.0.

```json
{
  "track": "surdo_1",
  "samples": {
    "open":    { "path": "surdo_aberto.wav", "midi_note": 36 },
    "muted":   { "path": "surdo_abafado.wav", "midi_note": 37 },
    "rim":     { "path": "surdo_aro.wav", "midi_note": 38 }
  },
  "pattern": {
    "samba": [
      { "step": 0, "sample": "muted", "vel": 0.4 },
      { "step": 4, "sample": "open", "vel": 1.0 },
      { "step": 8, "sample": "muted", "vel": 0.3 },
      { "step": 12, "sample": "open", "vel": 0.9 }
    ]
  }
}
```

---

### 5.1 Configuração de build

#### Plugin (CMake)

```cmake
cmake_minimum_required(VERSION 3.29)
project(lolooper VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(JUCE)  # JUCE como submódulo

juce_add_plugin(Lolooper
  COMPANY_NAME "passoz"
  PLUGIN_MANUFACTURER_CODE Lolo
  PLUGIN_CODE Lpr1
  FORMATS VST3 Standalone AU AAX
  PRODUCT_NAME "Lolooper"
  VERSION 1.0.0
  SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Sequencer.cpp
    Source/Mixer.cpp
    Source/SampleLib.cpp
)

# Target Android standalone (mesmo código)
juce_add_plugin(Lolooper_Android
  FORMATS Standalone
  PRODUCT_NAME "Lolooper"
  BUNDLE_ID "com.passoz.lolooper"
  COMPANY_NAME "passoz"
  SOURCES ${SOURCES}
)

# Target iOS standalone (mesmo código)
juce_add_plugin(Lolooper_iOS
  FORMATS Standalone
  PRODUCT_NAME "Lolooper"
  BUNDLE_ID "com.passoz.lolooper"
  COMPANY_NAME "passoz"
  SOURCES ${SOURCES}
)
```

#### MIDI no mobile

- **Android:** Android MIDI API via JUCE `MidiInput`/`MidiOutput`. Suporta USB e Bluetooth LE MIDI.
- **iOS:** CoreMIDI via JUCE. Suporta USB (Camera Connection Kit) e Bluetooth MIDI.
- O código C++ que processa MIDI **não muda** entre plataformas.

### Frontend (Vite)

```typescript
// vite.config.ts
import { defineConfig } from 'vite';
import vue from '@vitejs/plugin-vue';
import { VitePWA } from 'vite-plugin-pwa';

export default defineConfig({
  plugins: [
    vue(),
    VitePWA({
      registerType: 'autoUpdate',
      manifest: {
        name: 'Lolooper',
        short_name: 'Lolooper',
        display: 'standalone',
        background_color: '#0a0a0a',
        theme_color: '#1a1a2e',
      },
      workbox: {
        globPatterns: ['**/*.{js,css,html,ico,png,svg,woff2}'],
      },
    }),
  ],
});
```
