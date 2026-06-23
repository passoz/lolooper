# AGENTS.md — Instruções para agentes de IA

## Sobre este arquivo

Este documento descreve como agentes de IA (Claude, Copilot, Cursor, etc.) devem entender, navegar e contribuir com o projeto Lolooper. Leia **integralmente** antes de qualquer código.

---

## Arquitetura do projeto

O Lolooper tem **exatamente duas codebases** que não se sobrepõem:

```
PWA (Vue 3 + TypeScript)          Plugin (JUCE C++)
─────────────────────────          ──────────────────
Faz: edição, controle, UI, IA     Faz: áudio, sequenciador, samples
Roda em: Chrome/Edge               Roda em: VST3 (DAW), Standalone (Desktop/Android/iOS)
Comunica via: WebMIDI API          Comunica via: MIDI CC/Note/SysEx
Persiste em: IndexedDB             Persiste em: arquivo JSON
```

**Regra fundamental:** as duas codebases **nunca** se comunicam diretamente. Toda comunicação é MIDI.

**Multi-target:** o plugin compila para **todos** os targets do mesmo código C++:
- VST3 (Linux, Windows, Mac)
- Standalone Desktop (Linux, Windows, Mac)
- Standalone Android (.apk)
- Standalone iOS (.ipa)

---

## Estrutura de diretórios

```
lolooper/
├── plugin/          ← C++ / JUCE. Motor de áudio, VST3.
├── frontend/        ← Vue 3 / TypeScript. PWA, editor, controle.
├── docs/            ← Documentação de referência (patterns, MIDI, songs).
├── README.md        ← Visão geral do projeto.
├── SPEC.md          ← Especificação técnica detalhada.
├── TODO.md          ← Lista de tarefas priorizadas.
└── AGENTS.md        ← Este arquivo.
```

---

## Convenções de código

### Plugin (C++ / JUCE)
- C++17 (padrão JUCE)
- Nomes de classes: PascalCase (`LolooperProcessor`, `Sequencer`, `Mixer`)
- Nomes de métodos: camelCase (`processBlock`, `loadSample`, `setPattern`)
- Membros privados: prefixo `m_` (`m_currentStep`, `m_playing`)
- Um `.cpp` + `.h` por classe
- Incluir o header de copyright em todo arquivo novo
- Comentários em inglês

### Frontend (Vue 3 / TypeScript)
- TypeScript estrito (`strict: true`)
- Composition API com `<script setup lang="ts">`
- Pinia stores para estado global (patterns, songs, setlist, midi)
- Composables reutilizáveis em `src/composables/`
- Nomes de componentes: PascalCase (`PatternGrid.vue`, `TrackList.vue`)
- Funções e variáveis: camelCase (`cycleVelocity`, `midiOutput`)
- Tailwind CSS com glassmorphism (`backdrop-blur`, `bg-white/10`, `border-white/20`)
- Comentários em inglês

---

## Protocolo MIDI (PWA ↔ Plugin)

### CC (Control Change) — parâmetros contínuos

| CC# | Parâmetro | Range |
|---|---|---|
| 15 | BPM | 0-127 → 40-200 BPM |
| 16 | Swing | 0-127 → 0-100% |
| 17 | Humanize | 0-127 → 0-50ms |
| 18 | Pattern style | 0=samba, 1=pagode, 2=partido_alto, 3=samba_reggae, 4=ijexa, 5=frevo, 6=maracatu, 7=intro, 8=virada |
| 20-33 | Track volume (14 tracks) | 0-127 → 0.0-1.0 |
| 34-47 | Track pan (14 tracks) | 0-127 → 0.0-1.0 (0=left, 64=center, 127=right) |
| 48-61 | Track accent (14 tracks) | 0-127 → 0.0-2.0 |

### Note On/Off — ações discretas

| Note# | Ação |
|---|---|
| 36-49 | Toggle track mute (14 tracks) |
| 114 | Play |
| 115 | Stop |
| 116 | Pause |
| 117 | Record MIDI |
| 118 | Toggle metronome |
| 119 | Next scene |
| 120 | Previous scene |
| 121 | Trigger virada |
| 122 | Solo reset (unsolo all) |

### SysEx — dados grandes (patterns, songs)

```
SysEx header: 0xF0 0x7D 0xXX ...
  0x01 = Request patterns (PWA → Plugin)
  0x02 = Response patterns (Plugin → PWA)
  0x03 = Push patterns (PWA → Plugin)
  0x04 = Song structure update
```

---

## Fluxos de trabalho comuns

### Editar um pattern
1. Usuário edita no PWA (PatternGrid.vue clica célula)
2. Pinia store atualiza (`patternsStore.cycleVelocity(track, step)`)
3. Store persiste no IndexedDB automaticamente
4. Se o plugin estiver conectado, envia update via SysEx 0x03
5. Plugin recebe SysEx, atualiza pattern em memória, continua tocando

### Tocar uma música (song mode)
1. Usuário seleciona música no PWA ou IA decide
2. PWA envia estrutura da música via SysEx 0x04
3. Plugin carrega seções (intro 4 compassos → verso 16 → refrão 8...)
4. Plugin avança seções automaticamente (contagem de compassos)
5. PWA mostra progresso (beat/bar/seção atual)
6. Usuário pode pular seção via Note 119/120

### Gravar pattern via MIDI
1. PWA envia Note 117 (REC)
2. Plugin entra em modo gravação
3. Usuário toca pads MIDI
4. Plugin quantiza hits na grid do passo atual
5. PWA envia Note 117 novamente (STOP REC)
6. Plugin envia pattern resultante via SysEx 0x02
7. PWA atualiza grid visual

---

## Regras para agentes

1. **Nunca crie um servidor backend.** O projeto não tem Go, Python, Node.js server. O PWA é autocontido.

2. **Nunca crie comunicação HTTP/WebSocket entre PWA e plugin.** Use MIDI. Sempre.

3. **Nunca altere a stack de UI.** É Vue 3 + Pinia + Tailwind + Glassmorphism. Não sugira React, Svelte, ou outro framework.

4. **Mantenha as duas peças independentes.** Mudanças no plugin não devem quebrar o PWA, e vice-versa.

5. **Antes de qualquer código, leia SPEC.md e TODO.md.**

6. **Testes são obrigatórios.** Plugin: testes unitários com Catch2. PWA: testes com Vitest + Vue Test Utils.

7. **Todo texto de UI é em português** (i18n via `vue-i18n`). Código, comentários, logs e commits são em inglês.

8. **Commits atômicos e em inglês.** Formato: `[plugin]` ou `[pwa]` prefixo.

9. **Cada funcionalidade completada deve ser marcada no TODO.md.**

10. **Glassmorphism SEMPRE:** `backdrop-blur-lg bg-white/10 border border-white/20 shadow-xl rounded-2xl`
