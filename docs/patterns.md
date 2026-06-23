# docs/patterns.md — Referência de Padrões Rítmicos

## Estrutura de um pattern

Cada pattern é uma grid de **14 instrumentos × 16 semicolcheias** (1 compasso 4/4).

```
Velocidades:
  ● = 1.0 (forte, acento principal)
  ◉ = 0.7 (médio-forte)
  ○ = 0.4 (fraco, ghost note)
  · = 0.0 (silêncio)
```

## Instrumentos (ordem fixa)

| Índice | Nome | Descrição |
|---|---|---|
| 0 | surdo_1 | Surdo de primeira — marcação forte no 2º e 4º tempo |
| 1 | surdo_2 | Surdo de segunda — resposta no 1º e 3º tempo |
| 2 | surdo_3 | Surdo de terceira — cortador, viradas, síncope |
| 3 | caixa | Caixa — padrão característico de samba |
| 4 | repique | Repique — acentos fora do tempo, rolls |
| 5 | tamborim | Tamborim — semicolcheias constantes com acentos |
| 6 | pandeiro | Pandeiro — grave (polegar) + agudo (dedos) + platinela |
| 7 | cuica | Cuíca — chamadas melódicas esparsas |
| 8 | agogo | Agogô — alternância grave/agudo |
| 9 | reco_reco | Reco-reco — raspagem rítmica |
| 10 | tantan | Tantã — percussão melódica, grave |
| 11 | cavaquinho | Cavaquinho — harmonia e ritmo, strum |
| 12 | violao_7 | Violão 7 cordas — baixarias e acordes |
| 13 | banjo | Banjo cavaquinho — strum brilhante |

## Estilos disponíveis

| Estilo | BPM típico | Origem | Característica |
|---|---|---|---|
| **samba** | 90-120 | Rio de Janeiro | Padrão base. Surdo marca 2 e 4. |
| **pagode** | 80-100 | Rio/São Paulo | Mais lento, romântico. Tantã lidera. |
| **partido_alto** | 100-130 | Rio de Janeiro | Sincopado, pandeiro pesado. |
| **samba_reggae** | 70-90 | Bahia | Surdos no contratempo. Reggae + samba. |
| **ijexa** | 80-100 | Bahia | Toque de Iansã. Ritmo de afoxé. |
| **frevo** | 140-170 | Pernambuco | Rápido, metais, orquestra de rua. |
| **maracatu** | 80-110 | Pernambuco | Baque virado. Alfaias graves. |
| **intro** | — | — | Apenas surdo marcando. Para aberturas. |
| **virada** | — | — | Fill/break com todos os instrumentos. |

## Editando patterns

### No PWA (grid visual)
1. Clique numa célula para ciclar: · → ○ → ◉ → ● → ·
2. Arraste para selecionar múltiplas células
3. Teclas 1-4 para aplicar velocidade direta
4. Ctrl+C / Ctrl+V para copiar entre tracks
5. Ctrl+Z / Ctrl+Y para undo/redo

### Gravando via MIDI
1. Aperte REC (Note 117)
2. O sequenciador começa a rodar
3. Toque os pads do controlador — cada hit é quantizado no passo atual
4. Aperte REC novamente para parar
5. O pattern é enviado ao PWA via SysEx

### No arquivo JSON
```json
{
  "meu_samba": {
    "surdo_1": [0,0,0,0,1.0,0,0,0,0,0,0,0,1.0,0,0,0]
  }
}
```

## Dicas para música brasileira

- **Surdo 1** quase nunca toca no 1º tempo — a ênfase é no 2º (a "resposta" do samba)
- **Caixa** toca no contratempo — é o que dá o "balanço"
- **Tamborim** pode ser "carreteiro" (semicolcheias) ou "telecoteco" (padrão sincopado)
- **Pandeiro** combina grave (polegar no centro) e agudo (dedos na borda) — use multi-sample
- **Cuíca** entra esparsamente — não encha todos os passos
- **Cavaquinho** e **banjo** fazem o "strum" harmônico — o padrão de semicolcheias é essencial
- **Swing 10-15%** transforma um samba robótico em samba de raiz
- **Humanize 3-8ms** tira a sensação de "máquina" sem perder o groove
