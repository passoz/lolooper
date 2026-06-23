"""CLI entry point — start the Lolooper engine + API + MIDI.

Usage:
    lolooper                    # Start with defaults
    lolooper --config looper.yaml   # Start with config file
    lolooper --no-midi              # Start without MIDI
    lolooper --api-only             # API only, no audio engine
    lolooper --list-midi            # List MIDI ports and exit
    lolooper --list-styles          # List pattern styles and exit
"""

import logging
import signal
import sys
from pathlib import Path

import click

from . import __version__

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
    datefmt="%H:%M:%S",
)
logger = logging.getLogger("lolooper")


@click.command()
@click.option("--config", "-c", default="looper.yaml", help="Config file path")
@click.option("--no-midi", is_flag=True, help="Disable MIDI controller")
@click.option("--api-only", is_flag=True, help="Start API only (no audio engine)")
@click.option("--list-midi", is_flag=True, help="List MIDI ports and exit")
@click.option("--list-styles", is_flag=True, help="List pattern styles and exit")
@click.option("--port", "-p", default=8710, help="API port")
@click.option("--host", default="127.0.0.1", help="API host")
@click.option("--debug", is_flag=True, help="Enable debug logging")
@click.version_option(version=__version__)
def main(
    config: str,
    no_midi: bool,
    api_only: bool,
    list_midi: bool,
    list_styles: bool,
    port: int,
    host: str,
    debug: bool,
):
    """Lolooper — live performance looper for Brazilian samba & pagode.

    MIDI-controllable and AI-operable via HTTP API on http://127.0.0.1:8710
    """
    if debug:
        logging.getLogger().setLevel(logging.DEBUG)

    # ── Info-only modes ────────────────────────────────────

    if list_styles:
        from .patterns import list_styles, list_instruments

        click.echo("Available pattern styles:")
        for style in list_styles():
            instruments = list_instruments(style)
            click.echo(f"  {style}: {', '.join(instruments[:5])}{'...' if len(instruments) > 5 else ''}")
        return

    if list_midi:
        try:
            from .midi import MidiController

            inputs = MidiController.list_ports()
            outputs = MidiController.list_output_ports()
            click.echo("MIDI Input ports:")
            for p in inputs:
                click.echo(f"  → {p}")
            click.echo("\nMIDI Output ports:")
            for p in outputs:
                click.echo(f"  ← {p}")
            if not inputs and not outputs:
                click.echo("  (no MIDI ports found)")
        except Exception as e:
            click.echo(f"Error listing MIDI ports: {e}")
        return

    # ── Load config ────────────────────────────────────────

    from .config import load_config

    cfg_path = Path(config)
    cfg = load_config(cfg_path)
    cfg.api_port = port
    cfg.api_host = host

    click.echo(f"╔══════════════════════════════════════════╗")
    click.echo(f"║        🥁  Lolooper v{__version__}  🥁            ║")
    click.echo(f"╚══════════════════════════════════════════╝")
    click.echo(f"  BPM: {cfg.bpm}  |  Swing: {cfg.swing:.0%}  |  {cfg.beats_per_bar}/{cfg.bars_per_loop}")
    click.echo(f"  Tracks: {len(cfg.tracks)}  |  Samples: {cfg.samples_dir}")
    click.echo(f"  API: http://{cfg.api_host}:{cfg.api_port}")

    # ── API-only mode ──────────────────────────────────────

    if api_only:
        click.echo("\n  Mode: API only (no audio engine)")
        # Start just the API with a null engine
        _start_api_only(cfg)
        return

    # ── Full engine mode ───────────────────────────────────

    from .engine import LooperEngine
    from .api import create_app, set_engine
    from .midi import MidiController, MidiMapper

    click.echo(f"  Audio: {cfg.sample_rate}Hz / {cfg.block_size} samples")

    # Create engine
    engine = LooperEngine(
        sample_rate=cfg.sample_rate,
        block_size=cfg.block_size,
        bpm=cfg.bpm,
        swing=cfg.swing,
        beats_per_bar=cfg.beats_per_bar,
        bars_per_loop=cfg.bars_per_loop,
        samples_dir=cfg.samples_dir,
        audio_device=cfg.audio_device,
    )

    # Load tracks
    loaded_count = 0
    for tc in cfg.tracks:
        track = engine.add_track(
            name=tc.name,
            sample_path=tc.sample_path,
            volume=tc.volume,
            pan=tc.pan,
            muted=tc.muted,
            pattern_style=tc.pattern,
        )
        if track:
            loaded_count += 1
    click.echo(f"  Tracks loaded: {loaded_count}/{len(cfg.tracks)}")

    # Set engine for API
    set_engine(engine)

    # MIDI controller
    midi = None
    if not no_midi:
        try:
            midi = MidiController(port_name=cfg.midi_input_port)
            midi.start()
            # Wire MIDI to engine
            MidiMapper(engine, midi)
            click.echo(f"  MIDI: {midi.port_name}")
        except Exception as e:
            logger.warning(f"MIDI not available: {e}")
            click.echo(f"  MIDI: unavailable ({e})")
    else:
        click.echo("  MIDI: disabled")

    # ── Start API server ───────────────────────────────────

    import threading
    import uvicorn

    app = create_app()

    def run_api():
        uvicorn.run(app, host=cfg.api_host, port=cfg.api_port, log_level="warning")

    api_thread = threading.Thread(target=run_api, daemon=True)
    api_thread.start()

    click.echo(f"\n  ✓ API ready at http://{cfg.api_host}:{cfg.api_port}")
    click.echo(f"  ✓ Docs at http://{cfg.api_host}:{cfg.api_port}/docs")
    click.echo(f"\n  Commands:")
    click.echo(f"    curl -X POST http://{cfg.api_host}:{cfg.api_port}/transport/play")
    click.echo(f"    curl http://{cfg.api_host}:{cfg.api_port}/status")
    click.echo(f"    curl -X POST http://{cfg.api_host}:{cfg.api_port}/patterns/samba")
    click.echo(f"\n  Press Ctrl+C to stop\n")

    # ── Signal handling ────────────────────────────────────

    def shutdown(signum, frame):
        click.echo("\n  Shutting down...")
        engine.close()
        if midi:
            midi.stop()
        sys.exit(0)

    signal.signal(signal.SIGINT, shutdown)
    signal.signal(signal.SIGTERM, shutdown)

    # Auto-play
    engine.play()

    # Keep alive
    try:
        signal.pause()
    except AttributeError:
        # Windows doesn't have signal.pause
        import time
        while True:
            time.sleep(1)


def _start_api_only(cfg):
    """Start API without audio engine."""
    import threading
    import uvicorn

    from .api import create_app

    app = create_app()
    api_thread = threading.Thread(
        target=lambda: uvicorn.run(app, host=cfg.api_host, port=cfg.api_port, log_level="warning"),
        daemon=True,
    )
    api_thread.start()

    click.echo(f"\n  ✓ API at http://{cfg.api_host}:{cfg.api_port}")
    click.echo(f"  (no audio engine — /transport/* and /tracks/* will return 503)")

    try:
        signal.pause()
    except AttributeError:
        import time
        while True:
            time.sleep(1)


if __name__ == "__main__":
    main()
