# docs/song-format.md — Formato de Música (Song Mode)

## Estrutura

Uma música é composta por **seções** executadas em sequência. Cada seção tem um pattern, um número de compassos, e opcionalmente ações.

```yaml
song:
  name: "Nome da Música"
  bpm: 104
  key: "Dm"
  style: "samba"
  loop: false          # true = repete a última seção
  sections:
    - name: intro
      pattern: intro
      bars: 2

    - name: verso
      pattern: samba
      bars: 16
      actions:
        - at_bar: 0
          cmd: mute
          track: cavaquinho

    - name: pre_refrao
      pattern: samba_forte
      bars: 4
      actions:
        - at_bar: 0
          cmd: unmute_all
        - at_bar: 0
          cmd: set_accent
          track: pandeiro
          value: 1.2

    - name: refrao
      pattern: samba
      bars: 16
      actions:
        - at_bar: 0
          cmd: pattern
          style: samba_forte
        - at_bar: 0
          cmd: set_accent
          track: tamborim
          value: 1.3

    - name: virada
      pattern: virada
      bars: 1

    - name: fim
      pattern: intro
      bars: 4
      actions:
        - at_bar: 2
          cmd: fade_out
          duration: 2
```

## Campos

### Song

| Campo | Tipo | Descrição |
|---|---|---|
| `name` | string | Nome da música |
| `bpm` | float | Andamento base (20-300) |
| `key` | string | Tom (ex: "Cm", "F#m", "Am") |
| `style` | string | Estilo rítmico principal |
| `loop` | bool | Se true, repete a última seção indefinidamente |
| `sections` | array | Lista de seções em ordem |

### Section

| Campo | Tipo | Descrição |
|---|---|---|
| `name` | string | Nome da seção ("intro", "verso", "refrão", "ponte") |
| `pattern` | string | Nome do pattern a usar (deve existir na biblioteca) |
| `bars` | int | Número de compassos (1-64) |
| `actions` | array | Ações executadas no início da seção |

### Action

| Campo | Tipo | Descrição |
|---|---|---|
| `at_bar` | int | Em qual compasso da seção a ação ocorre (0 = primeiro) |
| `cmd` | string | Comando a executar |
| `track` | string | Track alvo (para mute, volume, accent, solo) |
| `value` | float | Valor (para volume, accent, pan) |
| `style` | string | Nome do pattern (para cmd: pattern) |
| `duration` | float | Duração em compassos (para fade_out) |

## Comandos disponíveis

| Comando | Track? | Value? | Descrição |
|---|---|---|---|
| `mute` | sim | não | Muta o track |
| `unmute` | sim | não | Desmuta o track |
| `unmute_all` | não | não | Desmuta todos os tracks |
| `solo` | sim | não | Solo no track |
| `unsolo_all` | não | não | Remove todos os solos |
| `set_volume` | sim | sim (0-1) | Ajusta volume |
| `set_pan` | sim | sim (0-1) | Ajusta pan |
| `set_accent` | sim | sim (0-2) | Ajusta accent |
| `pattern` | não | style | Troca o pattern de todos os tracks |
| `trigger_virada` | não | não | Toca virada imediatamente |
| `fade_out` | não | duration | Fade out gradual |
| `stop` | não | não | Para o transporte |

## Transições entre seções

O plugin automaticamente:
1. Conta os compassos da seção atual
2. Ao atingir `bars`, executa ações da **próxima** seção (`at_bar: 0`)
3. Troca o pattern para o da próxima seção
4. Continua tocando sem interrupção

Para pular uma seção antes do fim:
- PWA envia Note 119 (Next Scene)
- Plugin avança imediatamente para a próxima seção

## Exemplo: Samba de Roda completo

```yaml
song:
  name: "Samba de Roda"
  bpm: 104
  key: "Dm"
  style: "samba"
  sections:
    - name: intro
      pattern: intro
      bars: 4
    - name: verso
      pattern: samba
      bars: 16
      actions:
        - at_bar: 0
          cmd: mute
          track: cuica
    - name: refrao
      pattern: samba
      bars: 16
      actions:
        - at_bar: 0
          cmd: unmute
          track: cuica
        - at_bar: 0
          cmd: set_accent
          track: pandeiro
          value: 1.3
    - name: virada
      pattern: virada
      bars: 1
    - name: solo_cavaco
      pattern: samba
      bars: 8
      actions:
        - at_bar: 0
          cmd: solo
          track: cavaquinho
    - name: refrao_final
      pattern: samba
      bars: 16
      actions:
        - at_bar: 0
          cmd: unsolo_all
        - at_bar: 12
          cmd: fade_out
          duration: 4
```
