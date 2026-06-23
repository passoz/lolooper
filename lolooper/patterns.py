"""Pattern definitions for Brazilian samba and pagode rhythms.

Patterns are defined as grids of 16th notes (semiquavers) within a 4/4 bar.
Each cell is a velocity value: 0 = rest, 1.0 = full accent.
"""

# fmt: off
PATTERNS: dict[str, dict[str, list[float]]] = {
    # ================================================================
    # SAMBA (standard 2/4 feel, grid = 16 sixteenth notes per bar)
    # ================================================================
    "samba": {
        "surdo_1": [   # marcação forte — 2nd beat emphasis
            0.0, 0.0, 0.0, 0.0,  1.0, 0.0, 0.0, 0.0,  # beat 1 (wait) → beat 2 BUM
            0.0, 0.0, 0.0, 0.0,  1.0, 0.0, 0.0, 0.0,  # beat 3 (wait) → beat 4 BUM
        ],
        "surdo_2": [   # resposta — 1st beat
            1.0, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0,
            0.8, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0,
        ],
        "surdo_3": [   # virada / cortador — syncopated fills
            0.0, 0.0, 0.0, 0.8,  0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0,  0.0, 0.7, 0.6, 0.9,  # fill on last beats
        ],
        "caixa": [     # snare — standard samba pattern
            0.0, 0.0, 1.0, 0.0,  0.6, 0.0, 0.9, 0.0,
            0.0, 0.0, 1.0, 0.0,  0.6, 0.0, 0.8, 0.0,
        ],
        "repique": [   # repique — off-beat accents + rolls
            0.0, 0.0, 0.7, 0.0,  0.0, 0.0, 0.8, 0.0,
            0.0, 0.0, 0.6, 0.0,  0.0, 0.0, 0.9, 0.0,
        ],
        "tamborim": [  # tamborim — constant 16th with accents
            0.7, 0.4, 0.7, 0.4,  0.7, 0.4, 0.9, 0.5,
            0.7, 0.4, 0.7, 0.4,  0.9, 0.5, 1.0, 0.6,  # accent on last
        ],
        "pandeiro": [  # pandeiro — complex pattern with ghost notes
            0.6, 0.3, 0.8, 0.4,  0.5, 0.3, 0.7, 0.9,  # thumb (grave) + fingers
            0.6, 0.3, 0.8, 0.4,  0.5, 0.2, 1.0, 0.8,
        ],
        "cuica": [     # cuíca — sparse, melodic calls
            0.0, 0.0, 0.0, 0.0,  0.9, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.7, 0.0,  0.0, 0.6, 0.0, 0.0,
        ],
        "agogo": [     # agogô — high-low alternating
            0.8, 0.0, 0.6, 0.0,  0.8, 0.0, 1.0, 0.0,
            0.8, 0.0, 0.6, 0.0,  0.8, 0.0, 0.9, 0.0,
        ],
        "reco_reco": [  # reco-reco — scrape rhythm
            0.5, 0.4, 0.5, 0.4,  0.5, 0.4, 0.5, 0.4,
            0.5, 0.4, 0.5, 0.4,  0.5, 0.4, 0.5, 0.4,
        ],
        "tantan": [    # tantã — simpler, melodic percussion
            0.0, 0.0, 1.0, 0.0,  0.0, 0.5, 0.0, 0.0,
            0.0, 0.0, 0.8, 0.0,  0.0, 0.0, 0.4, 0.0,
        ],
        "cavaquinho": [  # cavaquinho — harmonic/rhythmic strum
            0.8, 0.4, 0.8, 0.4,  0.8, 0.4, 0.9, 0.5,
            0.8, 0.4, 0.8, 0.4,  0.9, 0.5, 1.0, 0.6,
        ],
        "violao_7": [  # violão 7 cordas — bass lines + chords
            0.9, 0.0, 0.0, 0.5,  0.0, 0.0, 0.6, 0.0,
            0.7, 0.0, 0.0, 0.4,  0.0, 0.0, 0.8, 0.0,
        ],
        "banjo": [     # banjo cavaquinho — bright strum
            0.7, 0.3, 0.7, 0.3,  0.8, 0.4, 0.7, 0.3,
            0.7, 0.3, 0.7, 0.3,  0.9, 0.4, 1.0, 0.5,
        ],
    },

    # ================================================================
    # PAGODE (slower, more romantic, emphasis on tantã and banjo)
    # ================================================================
    "pagode": {
        "surdo_1": [
            0.0, 0.0, 0.0, 0.0,  1.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0,  1.0, 0.0, 0.0, 0.0,
        ],
        "surdo_2": [
            0.7, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0,
            0.5, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0,
        ],
        "tantan": [    # tantã leads in pagode
            0.0, 0.0, 0.8, 0.5,  0.0, 0.0, 0.6, 0.4,
            0.0, 0.0, 0.9, 0.0,  0.0, 0.4, 0.6, 0.3,
        ],
        "pandeiro": [
            0.5, 0.2, 0.7, 0.3,  0.5, 0.2, 0.7, 0.6,
            0.5, 0.2, 0.7, 0.3,  0.5, 0.2, 0.8, 0.4,
        ],
        "tamborim": [
            0.5, 0.3, 0.5, 0.3,  0.6, 0.3, 0.7, 0.4,
            0.5, 0.3, 0.5, 0.3,  0.7, 0.4, 0.8, 0.5,
        ],
        "banjo": [     # banjo drives the harmony
            0.8, 0.3, 0.7, 0.4,  0.8, 0.3, 0.7, 0.5,
            0.8, 0.3, 0.7, 0.4,  0.9, 0.4, 1.0, 0.5,
        ],
        "cavaquinho": [
            0.7, 0.3, 0.7, 0.4,  0.7, 0.3, 0.8, 0.5,
            0.7, 0.3, 0.7, 0.4,  0.8, 0.5, 0.9, 0.5,
        ],
        "violao_7": [
            0.8, 0.0, 0.0, 0.4,  0.0, 0.0, 0.5, 0.0,
            0.6, 0.0, 0.0, 0.3,  0.0, 0.0, 0.7, 0.0,
        ],
        "reco_reco": [
            0.3, 0.3, 0.3, 0.3,  0.3, 0.3, 0.3, 0.3,
            0.3, 0.3, 0.3, 0.3,  0.3, 0.3, 0.3, 0.3,
        ],
        "cuica": [
            0.0, 0.0, 0.0, 0.0,  0.7, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.5,  0.0, 0.0, 0.0, 0.0,
        ],
    },

    # ================================================================
    # PARTIDO ALTO (more syncopated, pandeiro-driven, faster)
    # ================================================================
    "partido_alto": {
        "pandeiro": [
            0.6, 0.3, 0.9, 0.5,  0.6, 0.3, 0.8, 1.0,  # heavy pandeiro
            0.6, 0.3, 0.9, 0.5,  0.6, 0.3, 1.0, 0.8,
        ],
        "tamborim": [
            0.8, 0.5, 0.8, 0.5,  0.8, 0.5, 0.9, 0.6,
            0.8, 0.5, 0.8, 0.5,  1.0, 0.6, 0.9, 0.6,
        ],
        "surdo_1": [
            0.0, 0.0, 0.0, 0.0,  1.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0,  1.0, 0.0, 0.0, 0.0,
        ],
        "caixa": [
            0.0, 0.0, 1.0, 0.0,  0.5, 0.0, 0.9, 0.0,
            0.0, 0.0, 1.0, 0.0,  0.5, 0.0, 0.8, 0.0,
        ],
        "cuica": [
            0.0, 0.7, 0.0, 0.0,  0.9, 0.0, 0.0, 0.6,
            0.0, 0.5, 0.0, 0.8,  0.0, 0.0, 0.6, 0.0,
        ],
        "tantan": [
            0.0, 0.0, 0.7, 0.0,  0.0, 0.4, 0.0, 0.0,
            0.0, 0.0, 0.6, 0.0,  0.0, 0.0, 0.3, 0.0,
        ],
    },

    # ================================================================
    # SAMBA-REGGAE (Bahia style, heavier on surdos, slower)
    # ================================================================
    "samba_reggae": {
        "surdo_1": [
            1.0, 0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0,  # distinctive reggae offbeat
            1.0, 0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0,
        ],
        "surdo_2": [
            0.0, 0.0, 0.0, 0.8,  0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.6,  0.0, 0.0, 0.0, 0.0,
        ],
        "surdo_3": [
            0.8, 0.0, 0.0, 0.0,  0.0, 0.7, 0.0, 0.0,
            0.8, 0.0, 0.0, 0.0,  0.0, 0.9, 0.0, 0.0,
        ],
        "caixa": [
            0.0, 0.0, 0.8, 0.0,  0.5, 0.0, 0.8, 0.0,
            0.0, 0.0, 0.8, 0.0,  0.5, 0.0, 0.9, 0.0,
        ],
        "reco_reco": [
            0.4, 0.4, 0.4, 0.4,  0.4, 0.4, 0.4, 0.4,
            0.4, 0.4, 0.4, 0.4,  0.4, 0.4, 0.4, 0.4,
        ],
    },

    # ================================================================
    # INTRO (sparse, just surdo marking time)
    # ================================================================
    "intro": {
        "surdo_1": [
            0.0, 0.0, 0.0, 0.0,  0.8, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0,  0.8, 0.0, 0.0, 0.0,
        ],
        "cuica": [
            0.0, 0.0, 0.0, 0.0,  0.7, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0,  # sparse cuíca call
        ],
    },

    # ================================================================
    # VIRADA (fill/break — all instruments)
    # ================================================================
    "virada": {
        "surdo_1": [
            1.0, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.7, 0.5,  0.6, 0.7, 0.8, 1.0,  # roll leading to next section
        ],
        "surdo_3": [
            0.0, 0.8, 0.7, 0.9,  0.6, 0.8, 0.7, 0.9,
            0.8, 0.7, 0.9, 0.8,  1.0, 0.0, 0.0, 0.0,  # climax, then rest
        ],
        "caixa": [
            0.9, 0.0, 0.8, 0.0,  0.9, 0.0, 0.8, 0.0,
            0.9, 0.0, 0.8, 0.0,  1.0, 0.0, 1.0, 0.0,
        ],
        "repique": [
            0.0, 0.7, 0.0, 0.8,  0.0, 0.7, 0.0, 0.9,
            0.0, 0.8, 0.0, 0.9,  0.0, 1.0, 0.0, 0.0,
        ],
        "tamborim": [
            0.9, 0.6, 0.9, 0.6,  0.9, 0.6, 0.9, 0.6,
            0.9, 0.6, 0.9, 0.6,  1.0, 0.7, 1.0, 0.7,
        ],
    },
}
# fmt: on


EMPTY_PATTERN: list[float] = [0.0] * 16


def get_pattern(style: str, instrument: str) -> list[float]:
    """Get a pattern for a given style and instrument.

    Falls back to the samba pattern if the requested style doesn't exist,
    and returns an empty pattern if the instrument isn't found.
    """
    style_patterns = PATTERNS.get(style)
    if style_patterns is None:
        style_patterns = PATTERNS["samba"]
    return style_patterns.get(instrument, EMPTY_PATTERN.copy())


def list_styles() -> list[str]:
    """Return available style names."""
    return list(PATTERNS.keys())


def list_instruments(style: str = "samba") -> list[str]:
    """Return instrument names for a given style."""
    style_patterns = PATTERNS.get(style, PATTERNS["samba"])
    return list(style_patterns.keys())
