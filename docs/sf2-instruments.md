# Instrumentos SF2/SFZ no Lolooper

## Visão geral

A partir da Fase 7, o Lolooper suporta instrumentos harmônicos via SoundFont (SF2)
e SFZ, usando a biblioteca FluidSynth.

Isso permite que tracks melódicos (cavaquinho, violão 7 cordas, banjo) usem
samples reais multi-pitch em vez de samples únicos ou pitch shifting artificial.

## Pré-requisitos

- FluidSynth instalado (`libfluidsynth3` no Debian/Ubuntu)
- Um arquivo SF2 (ex: `TimGM6mb.sf2`, disponível no pacote `fluid-soundfont-gm`)
- Ou um SF2 de instrumento brasileiro (violão, cavaquinho, sanfona)

## Como usar

### 1. Obter um SoundFont

SoundFonts gratuitos de instrumentos brasileiros:

- **General MIDI**: `/usr/share/sounds/sf2/TimGM6mb.sf2`
- **Violão nylon**: buscar "nylon guitar sf2" online
- **Cavaquinho**: buscar "cavaquinho soundfont" online

### 2. Configurar no PWA (em desenvolvimento)

Na página de Configurações, selecione o arquivo SF2 para cada track melódico:

```
Track 11 (Cavaquinho)  → cavaquinho.sf2
Track 12 (Violão 7)    → violao_nylon.sf2
Track 13 (Banjo)       → banjo.sf2
```

### 3. Editar o pattern com graus (1-7)

No lugar de velocity, os tracks melódicos usam **graus do acorde**:

| Grau | Significado | Exemplo em C |
|------|------------|-------------|
| 1 | Tônica | C |
| 2 | Supertônica | D |
| 3 | Mediante | E |
| 4 | Subdominante | F |
| 5 | Dominante | G |
| 6 | Superdominante | A |
| 7 | Sensível | B |
| 0 | Pausa | — |

### 4. Definir a progressão de acordes

No Song Editor, cada seção tem uma progressão de acordes:

```
Seção "verso":
  Compassos: 16
  Acordes: C | C | F | F | G7 | G7 | C | C
```

O plugin automaticamente converte graus → notas conforme o acorde muda.

### 5. Na performance

1. Carregue os SF2 na inicialização
2. Crie patterns com graus (como se fosse velocity)
3. Defina os acordes da música no Song Editor
4. Toque! O baixo e harmonia seguem os acordes automaticamente

## Arquitetura

```
SF2/SFZ ──▶ FluidSynth ──▶ InstrumentLib ──▶ Mixer ──▶ Áudio
                  │
         noteOn(note, vel)  ←  Arpeggiator.getNoteForDegree(grau)
                                     ↑
                              ChordProgression.setChord("G7")
                                     ↑
                              PWA ── SysEx 0x08
```

## Referências

- FluidSynth: https://www.fluidsynth.org/
- SoundFonts gratuitos: https://sites.google.com/site/soundfonts4u/
- Tutoriais de SF2: https://www.fluidsynth.org/api/
