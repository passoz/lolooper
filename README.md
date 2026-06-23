# 🥁 Lolooper

**Live performance looper for Brazilian samba & pagode — MIDI + AI controllable.**

Lolooper is a looper/sequencer designed for live performance. Load samples of Brazilian percussion instruments, control them via MIDI controller during the show, and let an AI agent manage the arrangement via HTTP API.

## Features

- **Loop engine** — play samples in quantized 16th-note grids synced to BPM
- **14 instrument tracks** — surdo (1ª/2ª/3ª), caixa, repique, tamborim, pandeiro, cuíca, agogô, reco-reco, tantã, cavaquinho, violão 7 cordas, banjo
- **5 rhythm styles** — samba, pagode, partido alto, samba-reggae, intro/virada
- **MIDI control** — toggle tracks, adjust volume, tempo, swing via any MIDI controller
- **AI API** — full REST API for external control (AI agents, scripts, automation)
- **Live mixing** — per-track volume, pan, mute, solo
- **Transport** — play, stop, pause, BPM (20–300), swing (0–100%)
- **Per-track nuance** — accent multiplier, humanize setting

## Quick Start

```bash
# Install
pip install lolooper

# Run with defaults (no samples — add your own!)
lolooper

# Run with config
lolooper --config my_show.yaml

# List available MIDI ports
lolooper --list-midi

# List pattern styles
lolooper --list-styles
```

## AI Control (HTTP API)

The API runs on `http://127.0.0.1:8710` by default. Full OpenAPI docs at `/docs`.

```bash
# Transport
curl -X POST http://127.0.0.1:8710/transport/play
curl -X POST http://127.0.0.1:8710/transport/stop
curl -X PUT  http://127.0.0.1:8710/transport -H "Content-Type: application/json" -d '{"bpm": 98}'

# Tracks — toggle, volume, solo
curl -X POST http://127.0.0.1:8710/tracks/pandeiro/mute
curl -X POST http://127.0.0.1:8710/tracks/surdo_1/solo
curl -X PUT  http://127.0.0.1:8710/tracks/cuica -H "Content-Type: application/json" -d '{"volume": 0.7, "pan": 0.3}'

# Switch patterns (all tracks or per-track)
curl -X POST http://127.0.0.1:8710/patterns/pagode
curl -X POST http://127.0.0.1:8710/tracks/surdo_3/pattern/virada

# Full status
curl http://127.0.0.1:8710/status | jq
```

## Samples

Place `.wav` files in a `samples/` directory:

```
samples/
├── surdo_1.wav      # Surdo de primeira (marcação)
├── surdo_2.wav      # Surdo de segunda (resposta)
├── surdo_3.wav      # Surdo de terceira (virada)
├── caixa.wav        # Caixa (snare)
├── tamborim.wav     # Tamborim
├── pandeiro.wav     # Pandeiro
├── cuica.wav        # Cuíca
├── agogo.wav        # Agogô
├── reco_reco.wav    # Reco-reco
├── tantan.wav       # Tantã
├── repique.wav      # Repique
├── cavaquinho.wav   # Cavaquinho
├── violao_7.wav     # Violão 7 cordas
└── banjo.wav        # Banjo
```

## Configuration

Create a `looper.yaml`:

```yaml
bpm: 98
swing: 0.12
beats_per_bar: 4
bars_per_loop: 4
samples_dir: my_samples

midi_input_port: "MPK mini:MPK mini MIDI 1"

tracks:
  - name: surdo_1
    sample_path: my_samples/surdo_marcacao.wav
    volume: 0.9
    midi_note_on: 36
    pattern: samba
  - name: pandeiro
    sample_path: my_samples/pandeiro_close.wav
    volume: 0.75
    midi_note_on: 44
    pattern: samba
```

## MIDI Mapping

| Control | Type | Default |
|---------|------|---------|
| Play | Note 114 | C#8 |
| Stop | Note 115 | D8 |
| Track toggle | Notes 36–49 | C2–C#3 |
| Track volume | CC 20–33 | — |
| Tempo | CC 15 | 40–200 BPM |
| Swing | CC 16 | 0–100% |
| Master volume | CC 7 | — |

## Requirements

- Python 3.10+
- Linux / macOS / Windows
- Audio output device
- MIDI controller (optional — API works without one)

## License

MIT
