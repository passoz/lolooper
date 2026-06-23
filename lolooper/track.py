"""Track engine — individual instrument track with sample looping."""

import logging
from typing import Optional

import numpy as np

from .patterns import get_pattern, EMPTY_PATTERN

logger = logging.getLogger(__name__)


class Track:
    """A single instrument track that plays a sample in a rhythmic pattern.

    Each track holds one sample and plays it according to a 16-step grid
    (sixteenth notes) synchronized to the master transport.

    Parameters control:
    - volume: 0.0 to 1.0
    - pan: 0.0 (left) to 1.0 (right), 0.5 = center
    - muted: silence the track
    - solo: isolate the track (handled by mixer)
    - pattern: name of the rhythm pattern style
    """

    def __init__(
        self,
        name: str,
        sample_data: np.ndarray,
        sample_rate: int = 44100,
        volume: float = 1.0,
        pan: float = 0.5,
        muted: bool = False,
        solo: bool = False,
        pattern_style: str = "samba",
    ):
        self.name = name
        self.sample_data = sample_data.astype(np.float32)
        self.sample_rate = sample_rate
        self._volume = float(np.clip(volume, 0.0, 1.0))
        self._pan = float(np.clip(pan, 0.0, 1.0))
        self._muted = muted
        self._solo = solo
        self._active = True  # whether track is playing
        self._pattern_style = pattern_style
        self._pattern = get_pattern(pattern_style, name)

        # Per-step velocity (modified by accent/sensitivity)
        self.accent: float = 1.0  # global accent multiplier
        self.humanize: float = 0.0  # 0.0 = rigid, ~0.1 = subtle timing variation

    # ── Properties ──────────────────────────────────────────────

    @property
    def volume(self) -> float:
        return self._volume

    @volume.setter
    def volume(self, value: float):
        self._volume = float(np.clip(value, 0.0, 1.0))

    @property
    def pan(self) -> float:
        return self._pan

    @pan.setter
    def pan(self, value: float):
        self._pan = float(np.clip(value, 0.0, 1.0))

    @property
    def muted(self) -> bool:
        return self._muted

    @muted.setter
    def muted(self, value: bool):
        self._muted = bool(value)

    @property
    def solo(self) -> bool:
        return self._solo

    @solo.setter
    def solo(self, value: bool):
        self._solo = bool(value)

    @property
    def active(self) -> bool:
        return self._active

    @active.setter
    def active(self, value: bool):
        self._active = bool(value)

    @property
    def pattern_style(self) -> str:
        return self._pattern_style

    @pattern_style.setter
    def pattern_style(self, style: str):
        self._pattern_style = style
        self._pattern = get_pattern(style, self.name)
        logger.debug(f"Track {self.name}: pattern → {style}")

    # ── Audio generation ────────────────────────────────────────

    def render_step(self, step: int, n_samples: int) -> np.ndarray:
        """Render audio for one 16th-note step.

        Args:
            step: Step index (0–15) within the bar.
            n_samples: Number of audio samples in this step.

        Returns:
            np.ndarray of shape (n_samples,) mono or (n_samples, 2) stereo.
        """
        if not self._active or self._muted or len(self.sample_data) == 0:
            return np.zeros(n_samples, dtype=np.float32)

        velocity = self._pattern[step % 16] * self.accent
        if velocity <= 0.001:
            return np.zeros(n_samples, dtype=np.float32)

        # Get sample data, trim or pad to step length
        data = self.sample_data
        if len(data) > n_samples:
            data = data[:n_samples]
        elif len(data) < n_samples:
            data = np.pad(data, (0, n_samples - len(data)))

        # Apply velocity (volume envelope)
        data = data * velocity * self._volume

        # Apply pan (convert mono to stereo with pan)
        if data.ndim == 1:
            left_gain = 1.0 - self._pan
            right_gain = self._pan
            stereo = np.column_stack([data * left_gain, data * right_gain])
            return stereo
        else:
            # Already stereo — apply balance
            left_gain = 1.0 - self._pan
            right_gain = self._pan
            data[:, 0] *= left_gain
            data[:, 1] *= right_gain
            return data

    def to_dict(self) -> dict:
        """Return track state as a serializable dict (for API)."""
        return {
            "name": self.name,
            "volume": round(self._volume, 3),
            "pan": round(self._pan, 3),
            "muted": self._muted,
            "solo": self._solo,
            "active": self._active,
            "pattern": self._pattern_style,
            "accent": round(self.accent, 3),
            "humanize": round(self.humanize, 3),
        }
