"""Configuration management for Lolooper."""

from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

import yaml


@dataclass
class TrackConfig:
    """Configuration for a single instrument track."""

    name: str
    sample_path: str = ""
    volume: float = 1.0  # 0.0 to 1.0
    pan: float = 0.5  # 0.0 = left, 1.0 = right, 0.5 = center
    muted: bool = False
    solo: bool = False
    midi_channel: int = 0  # 0 = omni
    midi_note_on: int = 0  # MIDI note to toggle play
    midi_note_off: int = 0
    midi_cc_volume: int = 0  # MIDI CC for volume
    midi_cc_pan: int = 0  # MIDI CC for pan
    pattern: str = "default"


@dataclass
class LooperConfig:
    """Main configuration for Lolooper."""

    # Transport
    bpm: float = 100.0
    swing: float = 0.0  # 0.0 to 1.0 (0 = straight, ~0.16 = moderate swing)
    time_signature: tuple[int, int] = (4, 4)
    beats_per_bar: int = 4
    bars_per_loop: int = 4

    # Audio
    sample_rate: int = 44100
    block_size: int = 512
    audio_device: Optional[str] = None  # None = system default

    # MIDI
    midi_input_port: Optional[str] = None  # None = first available
    midi_output_port: Optional[str] = None
    midi_clock_send: bool = True
    midi_clock_receive: bool = False

    # API
    api_host: str = "127.0.0.1"
    api_port: int = 8710

    # Tracks
    tracks: list[TrackConfig] = field(default_factory=list)

    # Samples directory
    samples_dir: str = "samples"


def default_tracks() -> list[TrackConfig]:
    """Return default Brazilian percussion tracks."""
    instruments = [
        ("surdo_1", 36),  # C2 — kick/surdo de primeira
        ("surdo_2", 37),  # C#2 — surdo de segunda
        ("surdo_3", 38),  # D2 — surdo de terceira (virada)
        ("caixa", 39),  # D#2 — snare/caixa
        ("repique", 40),  # E2
        ("tamborim", 42),  # F#2
        ("pandeiro", 44),  # G#2
        ("cuica", 45),  # A2
        ("agogo", 46),  # A#2
        ("reco_reco", 47),  # B2
        ("tantan", 48),  # C3
        ("cavaquinho", 50),  # D3
        ("violao_7", 51),  # D#3
        ("banjo", 52),  # E3
    ]
    tracks = []
    for i, (name, note) in enumerate(instruments):
        t = TrackConfig(
            name=name,
            sample_path=f"samples/{name}.wav",
            midi_note_on=note,
            midi_cc_volume=20 + i,  # CC 20-33
            pattern="samba",
        )
        tracks.append(t)
    return tracks


def load_config(path: str | Path) -> LooperConfig:
    """Load configuration from a YAML file."""
    path = Path(path)
    if path.exists():
        with open(path) as f:
            data = yaml.safe_load(f) or {}
    else:
        data = {}

    cfg = LooperConfig()

    # Transport
    cfg.bpm = data.get("bpm", cfg.bpm)
    cfg.swing = data.get("swing", cfg.swing)
    cfg.beats_per_bar = data.get("beats_per_bar", cfg.beats_per_bar)
    cfg.bars_per_loop = data.get("bars_per_loop", cfg.bars_per_loop)

    # Audio
    cfg.sample_rate = data.get("sample_rate", cfg.sample_rate)
    cfg.block_size = data.get("block_size", cfg.block_size)
    cfg.audio_device = data.get("audio_device")

    # MIDI
    cfg.midi_input_port = data.get("midi_input_port")
    cfg.midi_output_port = data.get("midi_output_port")
    cfg.midi_clock_send = data.get("midi_clock_send", cfg.midi_clock_send)
    cfg.midi_clock_receive = data.get("midi_clock_receive", cfg.midi_clock_receive)

    # API
    cfg.api_host = data.get("api_host", cfg.api_host)
    cfg.api_port = data.get("api_port", cfg.api_port)

    # Samples
    cfg.samples_dir = data.get("samples_dir", cfg.samples_dir)

    # Tracks
    tracks_data = data.get("tracks")
    if tracks_data:
        cfg.tracks = [TrackConfig(**t) for t in tracks_data]
    else:
        cfg.tracks = default_tracks()

    return cfg


def save_config(cfg: LooperConfig, path: str | Path) -> None:
    """Save configuration to a YAML file."""
    path = Path(path)
    data = {
        "bpm": cfg.bpm,
        "swing": cfg.swing,
        "beats_per_bar": cfg.beats_per_bar,
        "bars_per_loop": cfg.bars_per_loop,
        "sample_rate": cfg.sample_rate,
        "block_size": cfg.block_size,
        "audio_device": cfg.audio_device,
        "midi_input_port": cfg.midi_input_port,
        "midi_output_port": cfg.midi_output_port,
        "midi_clock_send": cfg.midi_clock_send,
        "midi_clock_receive": cfg.midi_clock_receive,
        "api_host": cfg.api_host,
        "api_port": cfg.api_port,
        "samples_dir": cfg.samples_dir,
        "tracks": [
            {
                "name": t.name,
                "sample_path": t.sample_path,
                "volume": t.volume,
                "pan": t.pan,
                "muted": t.muted,
                "solo": t.solo,
                "midi_note_on": t.midi_note_on,
                "midi_cc_volume": t.midi_cc_volume,
                "pattern": t.pattern,
            }
            for t in cfg.tracks
        ],
    }
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w") as f:
        yaml.dump(data, f, default_flow_style=False, sort_keys=False)
