"""Sample management: loading, caching, and metadata for audio samples."""

import logging
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

import numpy as np
import soundfile as sf

logger = logging.getLogger(__name__)


@dataclass
class Sample:
    """A loaded audio sample with metadata."""

    name: str
    data: np.ndarray  # float32, shape (n_samples, n_channels) or (n_samples,)
    sample_rate: int
    path: Path
    duration: float  # in seconds

    @property
    def channels(self) -> int:
        if self.data.ndim == 1:
            return 1
        return self.data.shape[1]

    def slice(self, start_sample: int, end_sample: int) -> np.ndarray:
        """Get a slice of the sample."""
        return self.data[start_sample:end_sample]


class SampleLibrary:
    """Manages loading, caching, and accessing audio samples."""

    def __init__(self, base_dir: str | Path = "samples", target_sr: int = 44100):
        self.base_dir = Path(base_dir)
        self.target_sr = target_sr
        self._cache: dict[str, Sample] = {}
        self._missing: set[str] = set()

    def load(self, name: str, path: str | Path | None = None) -> Optional[Sample]:
        """Load a sample by name. Caches after first load."""
        if name in self._cache:
            return self._cache[name]
        if name in self._missing:
            return None

        if path is None:
            # Try common extensions
            for ext in (".wav", ".flac", ".ogg", ".mp3", ".aiff"):
                p = self.base_dir / f"{name}{ext}"
                if p.exists():
                    path = p
                    break
            else:
                path = self.base_dir / f"{name}.wav"
        else:
            path = Path(path)

        if not path.exists():
            logger.warning(f"Sample not found: {path}")
            self._missing.add(name)
            return None

        try:
            data, sr = sf.read(str(path), dtype="float32")
            if sr != self.target_sr:
                logger.info(f"Resampling {name} from {sr} to {self.target_sr}")
                # Simple resampling — for production, use samplerate or scipy
                from scipy.signal import resample

                ratio = self.target_sr / sr
                if data.ndim == 1:
                    data = resample(data, int(len(data) * ratio))
                else:
                    data = np.column_stack(
                        [resample(data[:, c], int(len(data) * ratio)) for c in range(data.shape[1])]
                    )

            duration = len(data) / self.target_sr
            sample = Sample(
                name=name,
                data=data.astype(np.float32),
                sample_rate=self.target_sr,
                path=path,
                duration=duration,
            )
            self._cache[name] = sample
            logger.info(f"Loaded sample: {name} ({duration:.2f}s, {sample.channels}ch)")
            return sample
        except Exception as e:
            logger.error(f"Failed to load sample {path}: {e}")
            self._missing.add(name)
            return None

    def preload(self, names: list[str]) -> dict[str, Optional[Sample]]:
        """Preload multiple samples. Returns dict of name -> Sample or None."""
        return {name: self.load(name) for name in names}

    def get(self, name: str) -> Optional[Sample]:
        """Get a loaded sample without trying to load it."""
        return self._cache.get(name)

    def list_loaded(self) -> list[str]:
        """Return names of all loaded samples."""
        return list(self._cache.keys())

    def list_available(self) -> list[str]:
        """List sample files available in base_dir."""
        if not self.base_dir.exists():
            return []
        samples = []
        for ext in (".wav", ".flac", ".ogg", ".mp3", ".aiff"):
            for p in self.base_dir.glob(f"*{ext}"):
                samples.append(p.stem)
        return sorted(set(samples))

    def clear(self) -> None:
        """Clear the sample cache."""
        self._cache.clear()
        self._missing.clear()
