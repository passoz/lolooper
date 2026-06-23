"""HTTP API — REST endpoints for AI and external control of Lolooper.

This is how an AI agent (or any external tool) controls the looper:
- Start/stop playback
- Toggle tracks (mute/unmute)
- Adjust volume, pan, BPM
- Switch patterns
- Query status
"""

import logging
from contextlib import asynccontextmanager
from typing import Optional, TYPE_CHECKING

from fastapi import FastAPI, HTTPException, Query
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field

from .patterns import list_styles, list_instruments

if TYPE_CHECKING:
    from .engine import LooperEngine

logger = logging.getLogger(__name__)

# ── Pydantic models ─────────────────────────────────────────────


class TrackUpdate(BaseModel):
    """Partial track update — all fields optional."""

    volume: Optional[float] = Field(None, ge=0.0, le=1.0)
    pan: Optional[float] = Field(None, ge=0.0, le=1.0)
    muted: Optional[bool] = None
    solo: Optional[bool] = None
    active: Optional[bool] = None
    pattern: Optional[str] = None
    accent: Optional[float] = Field(None, ge=0.0, le=2.0)
    humanize: Optional[float] = Field(None, ge=0.0, le=0.5)


class TransportUpdate(BaseModel):
    """Transport control update."""

    bpm: Optional[float] = Field(None, ge=20.0, le=300.0)
    swing: Optional[float] = Field(None, ge=0.0, le=1.0)


class TrackAdd(BaseModel):
    """Add a new track."""

    name: str
    sample_path: str = ""
    volume: float = Field(1.0, ge=0.0, le=1.0)
    pan: float = Field(0.5, ge=0.0, le=1.0)
    muted: bool = False
    pattern: str = "samba"


class StatusResponse(BaseModel):
    """Full status response."""

    playing: bool
    bpm: float
    swing: float
    step: int
    beat: int
    bar: int
    beats_per_bar: int
    bars_per_loop: int
    total_steps: int
    step_duration_ms: float
    any_solo: bool
    tracks: list[dict]
    loaded_samples: list[str]
    available_samples: list[str]
    available_styles: list[str]


# ── API application ─────────────────────────────────────────────

engine: Optional["LooperEngine"] = None


def set_engine(eng: "LooperEngine"):
    """Set the global engine reference for the API."""
    global engine
    engine = eng


def create_app() -> FastAPI:
    """Create and configure the FastAPI application."""

    @asynccontextmanager
    async def lifespan(app: FastAPI):
        logger.info("API server starting on port 8710")
        yield
        logger.info("API server shutting down")

    app = FastAPI(
        title="Lolooper API",
        description="Live performance looper for Brazilian samba & pagode — AI controllable",
        version="0.1.0",
        lifespan=lifespan,
    )

    # CORS — allow any origin for local control
    app.add_middleware(
        CORSMiddleware,
        allow_origins=["*"],
        allow_methods=["*"],
        allow_headers=["*"],
    )

    # ── Transport ──────────────────────────────────────────

    @app.post("/transport/play")
    async def transport_play():
        """Start playback."""
        if not engine:
            raise HTTPException(503, "Engine not initialized")
        engine.play()
        return {"action": "play", "playing": engine.playing}

    @app.post("/transport/stop")
    async def transport_stop():
        """Stop playback and reset position."""
        if not engine:
            raise HTTPException(503, "Engine not initialized")
        engine.stop()
        return {"action": "stop", "playing": engine.playing}

    @app.post("/transport/pause")
    async def transport_pause():
        """Pause playback without resetting position."""
        if not engine:
            raise HTTPException(503, "Engine not initialized")
        engine.pause()
        return {"action": "pause", "playing": engine.playing}

    @app.post("/transport/toggle")
    async def transport_toggle():
        """Toggle play/pause."""
        if not engine:
            raise HTTPException(503, "Engine not initialized")
        if engine.playing:
            engine.pause()
            return {"action": "pause", "playing": False}
        else:
            engine.play()
            return {"action": "play", "playing": True}

    @app.put("/transport")
    async def transport_update(update: TransportUpdate):
        """Update BPM and/or swing."""
        if not engine:
            raise HTTPException(503, "Engine not initialized")
        if update.bpm is not None:
            engine.bpm = update.bpm
        if update.swing is not None:
            engine.swing = update.swing
        return {"bpm": engine.bpm, "swing": engine.swing}

    # ── Tracks ─────────────────────────────────────────────

    @app.get("/tracks")
    async def list_tracks():
        """List all tracks."""
        if not engine:
            raise HTTPException(503, "Engine not initialized")
        return {"tracks": [t.to_dict() for t in engine.tracks.values()]}

    @app.get("/tracks/{name}")
    async def get_track(name: str):
        """Get a single track's state."""
        if not engine:
            raise HTTPException(503, "Engine not initialized")
        track = engine.get_track(name)
        if not track:
            raise HTTPException(404, f"Track '{name}' not found")
        return track.to_dict()

    @app.put("/tracks/{name}")
    async def update_track(name: str, update: TrackUpdate):
        """Update track parameters. All fields optional — only specified fields are changed."""
        if not engine:
            raise HTTPException(503, "Engine not initialized")
        track = engine.get_track(name)
        if not track:
            raise HTTPException(404, f"Track '{name}' not found")

        changed = {}
        if update.volume is not None:
            track.volume = update.volume
            changed["volume"] = track.volume
        if update.pan is not None:
            track.pan = update.pan
            changed["pan"] = track.pan
        if update.muted is not None:
            track.muted = update.muted
            changed["muted"] = track.muted
        if update.solo is not None:
            track.solo = update.solo
            changed["solo"] = track.solo
        if update.active is not None:
            track.active = update.active
            changed["active"] = track.active
        if update.pattern is not None:
            track.pattern_style = update.pattern
            changed["pattern"] = track.pattern_style
        if update.accent is not None:
            track.accent = update.accent
            changed["accent"] = track.accent
        if update.humanize is not None:
            track.humanize = update.humanize
            changed["humanize"] = track.humanize

        return {"track": name, "changed": changed}

    @app.post("/tracks/{name}/mute")
    async def mute_track(name: str):
        """Mute a track."""
        return await update_track(name, TrackUpdate(muted=True))

    @app.post("/tracks/{name}/unmute")
    async def unmute_track(name: str):
        """Unmute a track."""
        return await update_track(name, TrackUpdate(muted=False))

    @app.post("/tracks/{name}/toggle")
    async def toggle_track(name: str):
        """Toggle track mute state."""
        if not engine:
            raise HTTPException(503, "Engine not initialized")
        track = engine.get_track(name)
        if not track:
            raise HTTPException(404, f"Track '{name}' not found")
        track.muted = not track.muted
        return {"track": name, "muted": track.muted}

    @app.post("/tracks/{name}/solo")
    async def solo_track(name: str):
        """Solo a track (mutes all others)."""
        if not engine:
            raise HTTPException(503, "Engine not initialized")
        for t in engine.tracks.values():
            t.solo = (t.name == name)
        return {"solo": name}

    @app.post("/tracks/{name}/unsolo")
    async def unsolo_track(name: str):
        """Remove solo from a track (unmutes all)."""
        if not engine:
            raise HTTPException(503, "Engine not initialized")
        track = engine.get_track(name)
        if track:
            track.solo = False
        # Unmute all
        for t in engine.tracks.values():
            t.muted = False
        return {"solo": None}

    @app.post("/tracks/{name}/pattern/{style}")
    async def set_track_pattern(name: str, style: str):
        """Set the rhythm pattern style for a track (samba, pagode, partido_alto, etc.)."""
        if not engine:
            raise HTTPException(503, "Engine not initialized")
        track = engine.get_track(name)
        if not track:
            raise HTTPException(404, f"Track '{name}' not found")
        if style not in list_styles():
            raise HTTPException(400, f"Unknown style '{style}'. Available: {list_styles()}")
        track.pattern_style = style
        return {"track": name, "pattern": style}

    # ── Patterns ───────────────────────────────────────────

    @app.get("/patterns")
    async def get_patterns():
        """List available pattern styles."""
        return {"styles": list_styles()}

    @app.get("/patterns/{style}")
    async def get_pattern_style(style: str):
        """Get instruments available for a pattern style."""
        if style not in list_styles():
            raise HTTPException(400, f"Unknown style '{style}'. Available: {list_styles()}")
        return {"style": style, "instruments": list_instruments(style)}

    @app.post("/patterns/{style}")
    async def set_all_patterns(style: str):
        """Set all tracks to a pattern style."""
        if not engine:
            raise HTTPException(503, "Engine not initialized")
        if style not in list_styles():
            raise HTTPException(400, f"Unknown style '{style}'. Available: {list_styles()}")
        for track in engine.tracks.values():
            track.pattern_style = style
        return {"action": "set_all_patterns", "style": style}

    # ── Status ─────────────────────────────────────────────

    @app.get("/status")
    async def get_status() -> StatusResponse:
        """Get full engine status."""
        if not engine:
            raise HTTPException(503, "Engine not initialized")
        status = engine.get_status()
        status["available_styles"] = list_styles()
        return StatusResponse(**status)

    @app.get("/health")
    async def health():
        """Health check endpoint."""
        if not engine:
            return {"status": "no_engine"}
        return {"status": "ok", "playing": engine.playing, "tracks": len(engine.tracks)}

    # ── Samples ────────────────────────────────────────────

    @app.get("/samples")
    async def get_samples():
        """List loaded and available samples."""
        if not engine:
            raise HTTPException(503, "Engine not initialized")
        return {
            "loaded": engine.samples.list_loaded(),
            "available": engine.samples.list_available(),
        }

    @app.post("/samples/load/{name}")
    async def load_sample(name: str, path: str = Query("")):
        """Load a sample into the library."""
        if not engine:
            raise HTTPException(503, "Engine not initialized")
        sample = engine.samples.load(name, path if path else None)
        if sample:
            return {"loaded": name, "duration": sample.duration, "channels": sample.channels}
        raise HTTPException(404, f"Sample '{name}' not found")

    return app
