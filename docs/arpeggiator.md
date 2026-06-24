# Arpeggiador Harmônico — Estilo Teclado de Forró

## Conceito

O arpeggiador do Lolooper funciona como o acompanhamento automático de teclados
de forró: **a mão esquerda define o acorde, o padrão rítmico sai automático.**

No Lolooper:
- **Grid = padrão rítmico** (QUANDO tocar, fixo a música inteira)
- **Song sections = progressão de acordes** (QUAIS notas, muda a cada compasso)
- **Plugin = junta os dois em tempo real**

## Como funciona

### 1. Grid de graus

Para tracks melódicos (11-13), as células usam **graus (1-7)** em vez de velocity:

```
Baixo (violao_7):
┌────────────────────────────────────┐
│ step: 0  1  2  3  4  5  6  7  ... │
│ grau: 1  ·  ·  ·  5  ·  3  ·  ... │
│       ·  ·  ·  ·  3  ·  ·  ·  ... │
└────────────────────────────────────┘
```

### 2. Progressão de acordes

No Song Editor, cada seção tem acordes que mudam a cada compasso:

```
Seção "verso": C | C | F | F | G7 | G7 | C | C
```

### 3. O plugin em tempo real

```
Compasso 1, acorde = C (Dó maior)
  grau 1 = C, grau 3 = E, grau 5 = G
  → baixo toca: C · · · G · E · G · · · C · · · G · E · G

Compasso 3, acorde = F (Fá maior)
  grau 1 = F, grau 3 = A, grau 5 = C
  → baixo toca: F · · · C · A · C · · · F · · · C · A · C
```

**Mesmo desenho rítmico, notas diferentes conforme o acorde.**

## Tabela de graus por tipo de acorde

### Maior (C, D, E, F, G, A, B)

| Grau | Nota (em C) | Semitons da tônica |
|------|------------|-------------------|
| 1 | C (tônica) | 0 |
| 2 | D | 2 |
| 3 | E | 4 |
| 4 | F | 5 |
| 5 | G | 7 |
| 6 | A | 9 |
| 7 | B | 11 |

### Menor (Cm, Dm, Em, Fm, Gm, Am, Bm)

| Grau | Nota (em Cm) | Semitons da tônica |
|------|-------------|-------------------|
| 1 | C | 0 |
| 2 | D | 2 |
| 3 | Eb | 3 |
| 4 | F | 5 |
| 5 | G | 7 |
| 6 | Ab | 8 |
| 7 | Bb | 10 |

### Com sétima dominante (C7, D7, E7...)

Igual ao maior, mas grau 7 = 10 semitons (sétima menor).

## Exemplos de padrões de baixo

### Baião (tradicional)
```
1 · · · 5 · 3 · 5 · · · 1 · · · 5
```

### Forró universitário
```
1 · 5 · 1 · 5 · 1 · 5 · 3 · 5 ·
```

### Xote
```
1 · 3 5 · · 3 · · 1 · 3 5 · · 3 ·
```

## Progressões comuns

### Samba
```
C | C | F | F | C | G7 | C | C
```

### Baião
```
C | C | F | C | G7 | F | C | G7
```

### Forró maior
```
C | G7 | C | C | F | C | G7 | C
```

## Referência

- O sistema de graus é idêntico ao usado em teclados Yamaha PSR, teclados
  de forró (Roland E-09, etc.) e métodos de harmonia funcional
- Grau 0 = pausa (não toca nada naquele step)
- Oitava é automática (C3 para baixo, C4 para harmonia)
