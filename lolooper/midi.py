"""MIDI controller — detect, map, and process MIDI input for live control."""

import logging
import threading
from typing import Optional, Callable

try:
    import mido
    HAS_MIDO = True
except ImportError:
    HAS_MIDO = False

logger = logging.getLogger(__name__)

# Default MIDI mapping for transport + per-track control
# CC numbers: general controls
CC_TRANSPORT_PLAY = 114  # Play
CC_TRANSPORT_STOP = 115  # Stop
CC_TRANSPORT_PAUSE = 116  # Pause
CC_MASTER_VOLUME = 7  # Master volume
CC_TEMPO = 15  # Tempo (coarse)
CC_SWING = 16  # Swing amount

# Note numbers for track toggle (matches General MIDI drum map region)
NOTE_TRACK_BASE = 36  # Start of track toggle notes (C2)


class MidiController:
    """Listens for MIDI input and dispatches to callbacks.

    Handles:
    - Transport control (play/stop/pause via CC or Note)
    - Track muting/unmuting (Note on/off per track)
    - Volume control (CC per track)
    - Tempo control (CC)
    - Swing control (CC)
    """

    def __init__(self, port_name: Optional[str] = None):
        if not HAS_MIDO:
            raise RuntimeError("mido not installed. Install with: pip install mido python-rtmidi")

        self.port_name = port_name
        self._running = False
        self._thread: Optional[threading.Thread] = None
        self._input_port: Optional[mido.ports.BaseInput] = None

        # Callbacks — set these after construction
        self.on_note_on: Optional[Callable[[int, int], None]] = None  # (note, velocity)
        self.on_note_off: Optional[Callable[[int], None]] = None  # (note)
        self.on_cc: Optional[Callable[[int, int], None]] = None  # (cc_number, value)
        self.on_clock: Optional[Callable[[], None]] = None
        self.on_start: Optional[Callable[[], None]] = None
        self.on_stop: Optional[Callable[[], None]] = None

    @staticmethod
    def list_ports() -> list[str]:
        """List available MIDI input ports."""
        if not HAS_MIDO:
            return []
        return mido.get_input_names()

    @staticmethod
    def list_output_ports() -> list[str]:
        """List available MIDI output ports."""
        if not HAS_MIDO:
            return []
        return mido.get_output_names()

    def start(self):
        """Start listening for MIDI messages in a background thread."""
        if self._running:
            return

        try:
            backend = "mido.backends.rtmidi"
            mido.set_backend(backend)
        except Exception:
            logger.warning(f"Could not set rtmidi backend, using default")

        if self.port_name:
            try:
                self._input_port = mido.open_input(self.port_name)
            except Exception as e:
                available = self.list_ports()
                logger.error(
                    f"Could not open MIDI port '{self.port_name}': {e}"
                )
                if available:
                    logger.info(f"Available ports: {available}")
                return
        else:
            # Auto-detect first available port
            ports = self.list_ports()
            if not ports:
                logger.warning("No MIDI input ports available")
                return
            self.port_name = ports[0]
            self._input_port = mido.open_input(ports[0])

        self._running = True
        self._thread = threading.Thread(target=self._listen_loop, daemon=True)
        self._thread.start()
        logger.info(f"MIDI controller listening on: {self.port_name}")

    def stop(self):
        """Stop listening for MIDI messages."""
        self._running = False
        if self._input_port:
            try:
                self._input_port.close()
            except Exception:
                pass
            self._input_port = None
        if self._thread:
            self._thread.join(timeout=1.0)
            self._thread = None
        logger.info("MIDI controller stopped")

    def _listen_loop(self):
        """Background thread that reads MIDI messages."""
        while self._running and self._input_port:
            try:
                for msg in self._input_port.iter_pending():
                    self._handle_message(msg)
            except Exception as e:
                logger.error(f"MIDI read error: {e}")
                break
            # Small sleep to prevent busy-waiting
            import time
            time.sleep(0.001)

    def _handle_message(self, msg: "mido.Message"):
        """Dispatch a MIDI message to the appropriate callback."""
        try:
            if msg.type == "note_on" and msg.velocity > 0:
                if self.on_note_on:
                    self.on_note_on(msg.note, msg.velocity)
            elif msg.type == "note_off" or (msg.type == "note_on" and msg.velocity == 0):
                if self.on_note_off:
                    self.on_note_off(msg.note)
            elif msg.type == "control_change":
                if self.on_cc:
                    self.on_cc(msg.control, msg.value)
            elif msg.type == "clock":
                if self.on_clock:
                    self.on_clock()
            elif msg.type == "start":
                if self.on_start:
                    self.on_start()
            elif msg.type == "stop":
                if self.on_stop:
                    self.on_stop()
        except Exception as e:
            logger.debug(f"Error handling MIDI message {msg}: {e}")

    def send_clock(self):
        """Send a MIDI clock pulse (for clock master mode)."""
        # This is handled by the engine at the appropriate rate
        pass


class MidiMapper:
    """Maps MIDI messages to engine actions using the default mapping.

    This is the glue between raw MIDI and the LooperEngine.
    """

    def __init__(self, engine: "LooperEngine", midi: MidiController):
        from .engine import LooperEngine  # avoid circular import at type-check time

        self.engine = engine
        self.midi = midi
        self._note_to_track: dict[int, str] = {}
        self._cc_to_volume: dict[int, str] = {}

        # Build track mappings from engine config
        self._build_mappings()

        # Wire MIDI callbacks
        midi.on_note_on = self._handle_note_on
        midi.on_note_off = self._handle_note_off
        midi.on_cc = self._handle_cc
        midi.on_start = self._handle_midi_start
        midi.on_stop = self._handle_midi_stop

    def _build_mappings(self):
        """Build note→track and CC→track mappings from track config."""
        for track in self.engine.tracks.values():
            # Note toggle
            note = 36 + list(self.engine.tracks.keys()).index(track.name)
            self._note_to_track[note] = track.name

            # CC volume
            cc = 20 + list(self.engine.tracks.keys()).index(track.name)
            self._cc_to_volume[cc] = track.name

    def _handle_note_on(self, note: int, velocity: int):
        """Toggle track mute on note on."""
        track_name = self._note_to_track.get(note)
        if track_name:
            track = self.engine.get_track(track_name)
            if track:
                track.muted = not track.muted
                logger.info(f"MIDI Note {note}: {track_name} → muted={track.muted}")
        elif note == CC_TRANSPORT_PLAY:
            self.engine.play()
        elif note == CC_TRANSPORT_STOP:
            self.engine.stop()
        elif note == CC_TRANSPORT_PAUSE:
            self.engine.pause()

    def _handle_note_off(self, note: int):
        """Note off — no action (toggle is on note on only)."""
        pass

    def _handle_cc(self, cc: int, value: int):
        """Handle CC messages: volume per track, tempo, swing."""
        # Track volume
        track_name = self._cc_to_volume.get(cc)
        if track_name:
            track = self.engine.get_track(track_name)
            if track:
                track.volume = value / 127.0
                logger.debug(f"MIDI CC {cc}: {track_name} volume={track.volume:.2f}")
            return

        # Master controls
        if cc == CC_MASTER_VOLUME:
            vol = value / 127.0
            for track in self.engine.tracks.values():
                track.volume = vol
            logger.debug(f"MIDI CC {cc}: master volume={vol:.2f}")

        elif cc == CC_TEMPO:
            # Map 0-127 to 40-200 BPM
            bpm = 40 + (value / 127.0) * 160
            self.engine.bpm = bpm

        elif cc == CC_SWING:
            self.engine.swing = value / 127.0
            logger.debug(f"MIDI CC {cc}: swing={self.engine.swing:.2f}")

    def _handle_midi_start(self):
        """MIDI Start — begin playback."""
        self.engine.play()

    def _handle_midi_stop(self):
        """MIDI Stop — halt playback."""
        self.engine.stop()
