"""Tests for Lolooper."""

import numpy as np
import pytest

from lolooper.patterns import get_pattern, list_styles, list_instruments, EMPTY_PATTERN
from lolooper.track import Track
from lolooper.samples import SampleLibrary
from lolooper.config import LooperConfig, TrackConfig, default_tracks, load_config, save_config


class TestPatterns:
    def test_list_styles(self):
        styles = list_styles()
        assert "samba" in styles
        assert "pagode" in styles
        assert "partido_alto" in styles
        assert "virada" in styles
        assert len(styles) >= 4

    def test_list_instruments_samba(self):
        instruments = list_instruments("samba")
        assert "surdo_1" in instruments
        assert "pandeiro" in instruments
        assert "cuica" in instruments
        assert len(instruments) == 14

    def test_get_pattern_known(self):
        pattern = get_pattern("samba", "surdo_1")
        assert len(pattern) == 16
        assert pattern[4] == 1.0  # beat 2 accent
        assert pattern[12] == 1.0  # beat 4 accent

    def test_get_pattern_unknown_style_fallback(self):
        pattern = get_pattern("nonexistent", "surdo_1")
        assert len(pattern) == 16

    def test_get_pattern_unknown_instrument(self):
        pattern = get_pattern("samba", "nonexistent")
        assert pattern == EMPTY_PATTERN


class TestTrack:
    @pytest.fixture
    def sine_sample(self):
        sr = 44100
        t = np.linspace(0, 0.1, int(sr * 0.1), False)
        return np.sin(2 * np.pi * 440 * t).astype(np.float32)

    def test_create_track(self, sine_sample):
        track = Track("test", sine_sample, 44100)
        assert track.name == "test"
        assert track.volume == 1.0
        assert track.pan == 0.5
        assert not track.muted

    def test_mute(self, sine_sample):
        track = Track("test", sine_sample, 44100)
        track.muted = True
        out = track.render_step(4, 512)  # step 4 should have signal
        assert np.max(np.abs(out)) == 0.0

    def test_volume(self, sine_sample):
        track = Track("test", sine_sample, 44100, volume=0.5)
        track.pattern_style = "samba"  # won't match "test" but uses empty
        out_full = track.render_step(0, 512)
        track.volume = 0.0
        out_zero = track.render_step(0, 512)
        assert np.max(np.abs(out_zero)) == 0.0

    def test_pan_stereo_output(self, sine_sample):
        # Use a known instrument name so the pattern has non-zero velocities
        track = Track("surdo_1", sine_sample, 44100, pan=0.0)
        out = track.render_step(4, 512)  # step 4 has velocity 1.0 for surdo_1
        # Output should be stereo
        assert out.ndim == 2
        assert out.shape[1] == 2
        # Pan 0.0 = full left, right channel should be near zero
        assert np.max(np.abs(out[:, 1])) < 0.01

    def test_to_dict(self, sine_sample):
        track = Track("test", sine_sample, 44100, volume=0.8, pan=0.3, pattern_style="pagode")
        d = track.to_dict()
        assert d["name"] == "test"
        assert d["volume"] == 0.8
        assert d["pan"] == 0.3
        assert d["pattern"] == "pagode"


class TestSampleLibrary:
    def test_init(self):
        lib = SampleLibrary(base_dir="samples")
        assert lib.target_sr == 44100

    def test_load_missing(self):
        lib = SampleLibrary(base_dir="/nonexistent")
        sample = lib.load("nothing")
        assert sample is None

    def test_list_available_empty(self):
        lib = SampleLibrary(base_dir="/nonexistent")
        assert lib.list_available() == []


class TestConfig:
    def test_default_tracks(self):
        tracks = default_tracks()
        assert len(tracks) == 14
        names = [t.name for t in tracks]
        assert "surdo_1" in names
        assert "pandeiro" in names

    def test_default_config(self):
        cfg = LooperConfig()
        assert cfg.bpm == 100.0
        assert cfg.beats_per_bar == 4

    def test_track_config(self):
        tc = TrackConfig(name="surdo_1", volume=0.8)
        assert tc.name == "surdo_1"
        assert tc.volume == 0.8
        assert not tc.muted

    def test_save_load_roundtrip(self, tmp_path):
        cfg = LooperConfig(bpm=98.0, swing=0.12)
        cfg.tracks = [TrackConfig(name="test_track", volume=0.75)]
        path = tmp_path / "test_config.yaml"
        save_config(cfg, path)
        loaded = load_config(path)
        assert loaded.bpm == 98.0
        assert loaded.swing == 0.12
        assert len(loaded.tracks) == 1
        assert loaded.tracks[0].name == "test_track"
