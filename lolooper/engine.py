"""Core audio engine — transport, mixer, and sounddevice integration."""

import logging
import threading
import time
from collections import OrderedDict
from pathlib import Path
from typing import Optional

import numpy as np

try:
    import sounddevice as sd
    HAS_SOUNDDEVICE = True
except (ImportError, OSError) as e:
    HAS_SOUNDDEVICE = False
    _SD_ERROR = str(e)

from .samples import SampleLibrary
from .track import Track

logger = logging.getLogger(__name__)


class LooperEngine:
    """Main engine that coordinates tracks, transport, and audio output.

    Manages:
    - Transport: BPM, beat/step counting, playback state
    - Mixer: track volume, mute, solo logic
    - Audio stream: sounddevice callback-based output
    - Sample loading: via SampleLibrary
    """

    def __init__(
        self,
        sample_rate: int = 44100,
        block_size: int = 512,
        bpm: float = 100.0,
        swing: float = 0.0,
        beats_per_bar: int = 4,
        bars_per_loop: int = 4,
        samples_dir: str = "samples",
        audio_device: Optional[str] = None,
    ):
        if not HAS_SOUNDDEVICE:
            raise RuntimeError(
                f"sounddevice not available: {_SD_ERROR}. "
                "Install PortAudio: apt install libportaudio2"
            )

        self.sample_rate = sample_rate
        self.block_size = block_size
        self._bpm = bpm
        self.swing = swing
        self.beats_per_bar = beats_per_bar
        self.bars_per_loop = bars_per_loop

        # Transport state
        self._playing = False
        self._step = 0  # current step (0-15 within a bar)
        self._beat = 0  # current beat (0-3)
        self._bar = 0  # current bar (0-bars_per_loop-1)
        self._sample_pos = 0  # global sample counter
        self._stream: Optional[sd.OutputStream] = None
        self._lock = threading.Lock()

        # Sample library
        self.samples = SampleLibrary(base_dir=samples_dir, target_sr=sample_rate)

        # Tracks (ordered dict for consistent iteration)
        self.tracks: OrderedDict[str, Track] = OrderedDict()

        # Audio device
        self.audio_device = audio_device

        # Callbacks
        self.on_step: Optional[callable] = None  # called on each step advance
        self.on_beat: Optional[callable] = None  # called on each beat
        self.on_bar: Optional[callable] = None  # called on each bar

    # ── Transport properties ────────────────────────────────────

    @property
    def bpm(self) -> float:
        return self._bpm

    @bpm.setter
    def bpm(self, value: float):
        self._bpm = max(20.0, min(300.0, value))
        logger.info(f"BPM → {self._bpm:.1f}")

    @property
    def playing(self) -> bool:
        return self._playing

    @property
    def step(self) -> int:
        return self._step

    @property
    def beat(self) -> int:
        return self._beat

    @property
    def bar(self) -> int:
        return self._bar

    @property
    def steps_per_bar(self) -> int:
        return self.beats_per_bar * 4  # 16th notes per bar

    @property
    def total_steps(self) -> int:
        return self.steps_per_bar * self.bars_per_loop

    @property
    def step_duration(self) -> float:
        """Duration of one 16th-note step in seconds."""
        beat_duration = 60.0 / self._bpm
        return beat_duration / 4.0  # 16th note = quarter of a beat

    # ── Track management ────────────────────────────────────────

    def add_track(
        self,
        name: str,
        sample_path: str = "",
        volume: float = 1.0,
        pan: float = 0.5,
        muted: bool = False,
        pattern_style: str = "samba",
    ) -> Optional[Track]:
        """Add a track to the engine. Loads the sample if path is given."""
        sample_data = np.zeros(0, dtype=np.float32)
        if sample_path:
            sample = self.samples.load(name, sample_path)
            if sample:
                sample_data = sample.data
            else:
                logger.warning(f"Could not load sample for track '{name}', using silence")

        track = Track(
            name=name,
            sample_data=sample_data,
            sample_rate=self.sample_rate,
            volume=volume,
            pan=pan,
            muted=muted,
            pattern_style=pattern_style,
        )
        self.tracks[name] = track
        logger.info(f"Track added: {name}")
        return track

    def remove_track(self, name: str) -> bool:
        """Remove a track by name."""
        if name in self.tracks:
            del self.tracks[name]
            logger.info(f"Track removed: {name}")
            return True
        return False

    def get_track(self, name: str) -> Optional[Track]:
        """Get a track by name."""
        return self.tracks.get(name)

    # ── Transport control ───────────────────────────────────────

    def play(self):
        """Start playback."""
        if self._playing:
            return
        self._playing = True
        logger.info("▶ Transport: PLAY")
        self._ensure_stream()

    def stop(self):
        """Stop playback and reset position."""
        self._playing = False
        with self._lock:
            self._step = 0
            self._beat = 0
            self._bar = 0
            self._sample_pos = 0
        logger.info("■ Transport: STOP")

    def pause(self):
        """Pause playback without resetting position."""
        self._playing = False
        logger.info("⏸ Transport: PAUSE")

    def set_position(self, bar: int = 0, beat: int = 0, step: int = 0):
        """Set transport position."""
        with self._lock:
            self._bar = bar % self.bars_per_loop
            self._beat = beat % self.beats_per_bar
            self._step = step % self.steps_per_bar

    # ── Audio callback ──────────────────────────────────────────

    def _audio_callback(self, outdata: np.ndarray, frames: int, time_info, status):
        """Called by sounddevice for each audio block."""
        if status:
            logger.warning(f"Audio callback status: {status}")

        outdata.fill(0.0)
        if not self._playing:
            return

        with self._lock:
            bpm = self._bpm
            swing = self.swing
            step = self._step
            beat = self._beat
            bar = self._bar
            sample_pos = self._sample_pos

        sr = self.sample_rate
        step_dur_samples = int(sr * 60.0 / bpm / 4.0)  # 16th note in samples
        steps_per_bar = self.steps_per_bar
        total_steps = self.total_steps

        # Determine if any track is soloed
        any_solo = any(t.solo for t in self.tracks.values())

        # Process each step
        out_offset = 0
        while out_offset < frames:
            if step_dur_samples <= 0:
                break

            remaining_in_step = step_dur_samples - (sample_pos % step_dur_samples)
            if remaining_in_step <= 0:
                remaining_in_step = step_dur_samples

            n = min(frames - out_offset, remaining_in_step)

            # Mix all active tracks for this step
            for track in self.tracks.values():
                if any_solo and not track.solo:
                    continue
                if not track.active:
                    continue
                if track.muted:
                    continue

                rendered = track.render_step(step, n)
                if rendered.ndim == 1:
                    rendered = rendered.reshape(-1, 1)
                if rendered.shape[1] == 1 and outdata.shape[1] == 2:
                    rendered = np.column_stack([rendered[:, 0], rendered[:, 0]])
                if rendered.shape[0] != n:
                    rendered = rendered[:n]

                try:
                    outdata[out_offset : out_offset + n] += rendered
                except ValueError:
                    pass

            out_offset += n
            sample_pos += n

            # Advance step when we've played a full step
            if sample_pos % step_dur_samples == 0 or sample_pos >= step_dur_samples:
                old_step = step
                step = (step + 1) % steps_per_bar
                new_beat = step // 4
                new_bar = step // steps_per_bar if step == 0 and old_step > step else bar

                # Fire step callback
                if self.on_step:
                    try:
                        self.on_step(old_step, step)
                    except Exception:
                        pass

                # Fire beat callback
                if new_beat != beat:
                    beat = new_beat
                    if self.on_beat:
                        try:
                            self.on_beat(beat)
                        except Exception:
                            pass

                # Fire bar callback
                if step == 0 and old_step != 0:
                    bar = (bar + 1) % self.bars_per_loop
                    if self.on_bar:
                        try:
                            self.on_bar(bar)
                        except Exception:
                            pass

        # Update transport state
        with self._lock:
            self._step = step
            self._beat = beat
            self._bar = bar
            self._sample_pos = sample_pos

        # Soft clip to prevent distortion
        np.clip(outdata, -0.99, 0.99, out=outdata)

    # ── Stream management ───────────────────────────────────────

    def _ensure_stream(self):
        """Start the audio stream if not already running."""
        if self._stream is not None and self._stream.active:
            return

        try:
            self._stream = sd.OutputStream(
                samplerate=self.sample_rate,
                blocksize=self.block_size,
                device=self.audio_device,
                channels=2,
                dtype=np.float32,
                callback=self._audio_callback,
            )
            self._stream.start()
            logger.info("Audio stream started")
        except Exception as e:
            logger.error(f"Failed to start audio stream: {e}")
            self._playing = False
            raise

    def close(self):
        """Stop playback and close the audio stream."""
        self._playing = False
        if self._stream is not None:
            try:
                self._stream.stop()
                self._stream.close()
            except Exception:
                pass
            self._stream = None
        logger.info("Engine closed")

    # ── Status ──────────────────────────────────────────────────

    def get_status(self) -> dict:
        """Return full engine state as a dict (for API)."""
        return {
            "playing": self._playing,
            "bpm": round(self._bpm, 1),
            "swing": round(self.swing, 3),
            "step": self._step,
            "beat": self._beat,
            "bar": self._bar,
            "beats_per_bar": self.beats_per_bar,
            "bars_per_loop": self.bars_per_loop,
            "total_steps": self.total_steps,
            "step_duration_ms": round(self.step_duration * 1000, 1),
            "sample_rate": self.sample_rate,
            "block_size": self.block_size,
            "tracks": [t.to_dict() for t in self.tracks.values()],
            "any_solo": any(t.solo for t in self.tracks.values()),
            "loaded_samples": self.samples.list_loaded(),
            "available_samples": self.samples.list_available(),
        }
