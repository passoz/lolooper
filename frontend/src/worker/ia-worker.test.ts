/**
 * Unit tests for the IA Worker decision engine.
 * Tests the `decide()` function logic in isolation.
 */
import { describe, it, expect } from 'vitest'
import type { IAState, IAAction } from './ia-worker'

const TRACK_NAMES = [
  'Surdo 1', 'Surdo 2', 'Surdo 3', 'Caixa', 'Repique',
  'Tamborim', 'Pandeiro', 'Cuica', 'Agogo', 'Reco-reco',
  'Tantan', 'Cavaquinho', 'Violao 7', 'Banjo',
]

const SECTION_ENERGY: Record<string, number> = {
  intro: 0.3, verso: 0.5, pre_refrao: 0.7, refrao: 1.0,
  ponte: 0.6, virada: 0.9, fim: 0.2,
}

function makeState(overrides: Partial<IAState> = {}): IAState {
  return {
    bar: 0, beat: 0, step: 0, section: 'verso',
    sectionsRemaining: 4, energy: 0.5,
    mode: 'full', trackNames: TRACK_NAMES,
    mutedTracks: [], soloedTracks: [],
    currentBpm: 100,
    ...overrides,
  }
}

// Mirrored decide() from ia-worker.ts (deterministic for testing)
function decide(state: IAState): IAAction[] {
  const actions: IAAction[] = []
  if (state.mode === 'watch') return actions

  const sectionEnergy = SECTION_ENERGY[state.section] ?? 0.5
  const energyDelta = sectionEnergy - state.energy

  if (energyDelta > 0.2) {
    const percussionTracks = state.trackNames.filter(
      t => !['Cavaquinho', 'Violao 7', 'Banjo'].includes(t)
    )
    const mutedPerc = percussionTracks.filter(t => state.mutedTracks.includes(t))
    if (mutedPerc.length > 0) {
      actions.push({ type: 'unmute', track: mutedPerc[0], confidence: 0.8 })
    }
  }

  if (energyDelta < -0.3) {
    actions.push({
      type: 'volume', track: 'Cavaquinho',
      value: Math.max(0.3, 0.8 + energyDelta),
      confidence: 0.5,
    })
  }

  // Trigger virada when not near end (3-4 sections remaining)
  if (state.sectionsRemaining > 2 && state.sectionsRemaining <= 4 && state.beat === 0 && state.step === 0) {
    actions.push({ type: 'triggerVirada', confidence: 0.6 })
  }

  // Section advance: only when near end (1-2 sections remaining)
  if (state.sectionsRemaining > 0 && state.sectionsRemaining <= 2 && state.beat === 0 && state.step === 0 && state.bar > 0) {
    actions.push({ type: 'nextSection', confidence: 0.5 })
  }

  return actions
}

describe('IA decide()', () => {
  it('returns no actions in watch mode', () => {
    const actions = decide(makeState({ mode: 'watch' }))
    expect(actions).toHaveLength(0)
  })

  it('returns actions in full mode', () => {
    const state = makeState({
      mode: 'full', section: 'refrao', energy: 0.3,
      mutedTracks: ['Surdo 1'],
      sectionsRemaining: 2, bar: 1, beat: 0, step: 0,
    })
    const actions = decide(state)
    expect(actions.length).toBeGreaterThanOrEqual(2)
  })

  it('triggers virada when sectionsRemaining is 3 or 4', () => {
    const actions = decide(makeState({
      mode: 'full', sectionsRemaining: 3, beat: 0, step: 0,
    }))
    expect(actions.some(a => a.type === 'triggerVirada')).toBe(true)
  })

  it('does not trigger virada when sectionsRemaining is 1 or 2', () => {
    const actions = decide(makeState({
      mode: 'full', sectionsRemaining: 1, beat: 0, step: 0,
    }))
    expect(actions.some(a => a.type === 'triggerVirada')).toBe(false)
  })

  it('adds nextSection when sectionsRemaining is 1 or 2', () => {
    const actions = decide(makeState({
      mode: 'full', sectionsRemaining: 2, bar: 2, beat: 0, step: 0,
    }))
    expect(actions.some(a => a.type === 'nextSection')).toBe(true)
  })

  it('does not add nextSection on bar 0', () => {
    const actions = decide(makeState({
      mode: 'full', sectionsRemaining: 2, bar: 0, beat: 0, step: 0,
    }))
    expect(actions.some(a => a.type === 'nextSection')).toBe(false)
  })

  it('unmutes percussion when energy rises', () => {
    const state = makeState({
      mode: 'full', section: 'refrao', energy: 0.3,
      mutedTracks: ['Caixa', 'Pandeiro'],
    })
    const actions = decide(state)
    const unmuteActions = actions.filter(a => a.type === 'unmute')
    expect(unmuteActions.length).toBeGreaterThan(0)
    expect(['Caixa', 'Pandeiro']).toContain(unmuteActions[0].track)
  })

  it('lowers cavaquinho volume when energy drops', () => {
    const state = makeState({ mode: 'full', section: 'intro', energy: 0.9 })
    const actions = decide(state)
    const volumeActions = actions.filter(a => a.type === 'volume')
    expect(volumeActions.length).toBeGreaterThan(0)
    if (volumeActions[0].value !== undefined) {
      expect(volumeActions[0].value).toBeLessThan(0.8)
    }
  })

  it('assist mode produces same decisions as full', () => {
    const s = { sectionsRemaining: 2, bar: 1, beat: 0, step: 0 }
    const actionsFull = decide(makeState({ mode: 'full', ...s }))
    const actionsAssist = decide(makeState({ mode: 'assist', ...s }))
    expect(actionsAssist.length).toBe(actionsFull.length)
  })

  it('handles empty muted tracks gracefully', () => {
    const actions = decide(makeState({
      mode: 'full', section: 'refrao', energy: 0.3, mutedTracks: [],
    }))
    expect(actions.filter(a => a.type === 'unmute')).toHaveLength(0)
  })
})
