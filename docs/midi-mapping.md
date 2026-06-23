# docs/midi-mapping.md — MIDI Mapping Reference

Toda comunicação entre PWA e plugin é via MIDI. Este documento lista todo o mapeamento.

---

## Control Change (CC) — parâmetros contínuos

### Transport

| CC# | Parâmetro | Range | Descrição |
|---|---|---|---|
| 15 | BPM | 0-127 → 40-200 | Andamento |
| 16 | Swing | 0-127 → 0-100% | Deslocamento rítmico |
| 17 | Humanize | 0-127 → 0-50ms | Variação aleatória de timing |
| 18 | Pattern Style | 0-8 | 0=samba, 1=pagode, 2=partido_alto, 3=samba_reggae, 4=ijexa, 5=frevo, 6=maracatu, 7=intro, 8=virada |

### Track Volume (14 tracks)

| CC# | Track |
|---|---|
| 20 | surdo_1 |
| 21 | surdo_2 |
| 22 | surdo_3 |
| 23 | caixa |
| 24 | repique |
| 25 | tamborim |
| 26 | pandeiro |
| 27 | cuica |
| 28 | agogo |
| 29 | reco_reco |
| 30 | tantan |
| 31 | cavaquinho |
| 32 | violao_7 |
| 33 | banjo |

Range: 0-127 → 0.0-1.0

### Track Pan (14 tracks)

| CC# | Track |
|---|---|
| 34 | surdo_1 |
| 35 | surdo_2 |
| 36 | surdo_3 |
| 37 | caixa |
| 38 | repique |
| 39 | tamborim |
| 40 | pandeiro |
| 41 | cuica |
| 42 | agogo |
| 43 | reco_reco |
| 44 | tantan |
| 45 | cavaquinho |
| 46 | violao_7 |
| 47 | banjo |

Range: 0-127 → 0.0-1.0 (0=left, 64=center, 127=right)

### Track Accent (14 tracks)

| CC# | Track |
|---|---|
| 48-61 | surdo_1 a banjo |

Range: 0-127 → 0.0-2.0 (default 1.0)

---

## Note On/Off — ações discretas

### Track Mute Toggle (14 tracks)

| Note# | Track |
|---|---|
| 36 (C2) | surdo_1 |
| 37 (C#2) | surdo_2 |
| 38 (D2) | surdo_3 |
| 39 (D#2) | caixa |
| 40 (E2) | repique |
| 41 (F2) | tamborim |
| 42 (F#2) | pandeiro |
| 43 (G2) | cuica |
| 44 (G#2) | agogo |
| 45 (A2) | reco_reco |
| 46 (A#2) | tantan |
| 47 (B2) | cavaquinho |
| 48 (C3) | violao_7 |
| 49 (C#3) | banjo |

Velocity > 0 = toggle mute. Note Off ignorado.

### Transport

| Note# | Ação |
|---|---|
| 114 (C#8) | Play |
| 115 (D8) | Stop |
| 116 (D#8) | Pause |
| 117 (E8) | Record MIDI (toggle) |
| 118 (F8) | Metronome (toggle) |

### Scene / Section

| Note# | Ação |
|---|---|
| 119 (F#8) | Next scene |
| 120 (G8) | Previous scene |
| 121 (G#8) | Trigger virada |
| 122 (A8) | Solo reset (unsolo all) |

---

## SysEx — dados grandes

### Header

Todos os SysEx do Lolooper usam: `F0 7D XX ... F7`

| Byte 3 (XX) | Direção | Descrição |
|---|---|---|
| 01 | PWA → Plugin | Request: me mande todos os patterns |
| 02 | Plugin → PWA | Response: aqui estão os patterns (JSON) |
| 03 | PWA → Plugin | Push: substitua este pattern agora |
| 04 | PWA → Plugin | Push: estrutura da música (song mode) |

### SysEx 0x01 — Request patterns
```
F0 7D 01 F7
```
Plugin responde com SysEx 0x02.

### SysEx 0x02 — Response patterns
```
F0 7D 02 <JSON bytes> F7
```
JSON: `{"samba": {...}, "pagode": {...}, ...}`

### SysEx 0x03 — Push pattern
```
F0 7D 03 <JSON bytes> F7
```
JSON: `{"style": "samba", "grid": [[...], [...], ...]}`

Plugin substitui o pattern em tempo real, sem interromper o áudio.

### SysEx 0x04 — Push song structure
```
F0 7D 04 <JSON bytes> F7
```
JSON: estrutura completa da música (nome, BPM, seções).
