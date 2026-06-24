/**
 * IA Engine — Web Worker for Lolooper.
 * Runs in a separate thread. Receives state from the PWA,
 * decides actions based on configurable strategies.
 *
 * Modes:
 *   watch   — only monitors, never acts
 *   assist  — suggests actions (PWA must confirm)
 *   full    — acts automatically
 */
export interface IAState {
  bar: number
  beat: number
  step: number
  section: string
  sectionsRemaining: number
  energy: number          // 0-1, computed from current state
  mode: 'watch' | 'assist' | 'full'
  trackNames: string[]
  mutedTracks: string[]
  soloedTracks: string[]
  currentBpm: number
}

export interface IAAction {
  type: 'mute' | 'unmute' | 'volume' | 'pattern' | 'triggerVirada' | 'nextSection'
  track?: string
  value?: number
  confidence?: number     // 0-1: how confident the IA is
}

// Default strategies
interface IAStrategy {
  name: string
  description: string
  enabled: boolean
}

const DEFAULT_STRATEGIES: IAStrategy[] = [
  { name: 'energy_management', description: 'Aumenta accent no refrão, reduz no verso', enabled: true },
  { name: 'variation', description: 'Alterna patterns similares pra evitar repetição', enabled: true },
  { name: 'section_detection', description: 'Reconhece seções e aplica ações pré-definidas', enabled: true },
  { name: 'virada_trigger', description: 'Dispara viradas automaticamente em transições', enabled: true },
]

// Track energy levels for different sections
const SECTION_ENERGY: Record<string, number> = {
  intro: 0.3,
  verso: 0.5,
  pre_refrao: 0.7,
  refrao: 1.0,
  ponte: 0.6,
  virada: 0.9,
  fim: 0.2,
}

function decide(state: IAState): IAAction[] {
  const actions: IAAction[] = []
  if (state.mode === 'watch') return actions

  // Energy management: adjust accent/volume based on section energy
  if (DEFAULT_STRATEGIES[0].enabled) {
    const energyActions = decideEnergy(state)
    actions.push(...energyActions)
  }

  // Section transitions: detect pattern changes
  // Only trigger virada when many sections remain (not near end)
  if (state.sectionsRemaining > 2 && state.sectionsRemaining <= 4 && state.beat === 0 && state.step === 0) {
    if (DEFAULT_STRATEGIES[3].enabled) {
      actions.push({
        type: 'triggerVirada' as const,
        confidence: 0.6,
      })
    }
  }

  // Automatic section advance when sections remain
  // Don't also fire virada when near the end — advance takes priority
  if (state.sectionsRemaining > 0 && state.beat === 0 && state.step === 0 && state.bar > 0) {
    // Only advance if we're near the end (last 2 sections)
    if (state.sectionsRemaining <= 2) {
      actions.push({
        type: 'nextSection' as const,
        confidence: 0.5,
      })
    }
  }

  return actions
}

function decideEnergy(state: IAState): IAAction[] {
  const actions: IAAction[] = []
  const sectionEnergy = SECTION_ENERGY[state.section] ?? 0.5
  const energyDelta = sectionEnergy - state.energy

  // If section is higher energy than current, unmute tracks and increase volume
  if (energyDelta > 0.2) {
    // Unmute a random muted percussion track (not cavaquinho/violao)
    const percussionTracks = state.trackNames.filter(
      t => !['Cavaquinho', 'Violao 7', 'Banjo'].includes(t)
    )
    const mutedPerc = percussionTracks.filter(t => state.mutedTracks.includes(t))
    if (mutedPerc.length > 0 && Math.random() < 0.3) {
      actions.push({
        type: 'unmute' as const,
        track: mutedPerc[Math.floor(Math.random() * mutedPerc.length)],
        confidence: 0.6 + Math.random() * 0.3,
      })
    }
  }

  // If section energy drops, reduce volume on cavaquinho/violao for a sparser feel
  if (energyDelta < -0.3) {
    actions.push({
      type: 'volume' as const,
      track: 'Cavaquinho',
      value: Math.max(0.3, 0.8 + energyDelta),
      confidence: 0.5,
    })
  }

  return actions
}

// Worker message handler
self.onmessage = (e: MessageEvent<IAState>) => {
  const actions = decide(e.data)
  if (actions.length > 0) {
    self.postMessage(actions)
  }
}
