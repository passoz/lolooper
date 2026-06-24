/**
 * Unit tests for the patterns Pinia store.
 */
import { describe, it, expect, beforeEach, vi } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { usePatternsStore } from './patterns'

// Mock the database module
vi.mock('../db/database', () => ({
  db: {
    loadPatterns: vi.fn().mockResolvedValue(null),
    savePatterns: vi.fn().mockResolvedValue(undefined),
  },
}))

describe('usePatternsStore', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('initializes with empty styles and active style "samba"', () => {
    const store = usePatternsStore()
    expect(store.activeStyle).toBe('samba')
    expect(store.styles.size).toBe(0)
    expect(store.activePattern).toBeUndefined()
  })

  it('addStyle creates a new style with empty grid', () => {
    const store = usePatternsStore()
    store.addStyle('frevo')
    expect(store.styles.has('frevo')).toBe(true)
    expect(store.activeStyle).toBe('frevo')
    const pattern = store.styles.get('frevo')!
    expect(pattern.grid.length).toBe(14)
    expect(pattern.grid[0].length).toBe(16)
    expect(pattern.grid[0][0]).toBe(0)
  })

  it('addStyle does not overwrite existing style', () => {
    const store = usePatternsStore()
    store.addStyle('samba')
    store.setVelocity(0, 0, 0.7)
    store.addStyle('samba') // should be no-op
    expect(store.styles.get('samba')!.grid[0][0]).toBe(0.7)
  })

  it('cycleVelocity cycles through 4 velocity states', () => {
    const store = usePatternsStore()
    store.addStyle('test')
    const pattern = store.styles.get('test')!

    // Start at 0
    expect(pattern.grid[0][0]).toBe(0)

    store.cycleVelocity(0, 0)
    expect(pattern.grid[0][0]).toBe(0.4)

    store.cycleVelocity(0, 0)
    expect(pattern.grid[0][0]).toBe(0.7)

    store.cycleVelocity(0, 0)
    expect(pattern.grid[0][0]).toBe(1.0)

    store.cycleVelocity(0, 0)
    expect(pattern.grid[0][0]).toBe(0) // wraps around
  })

  it('setVelocity sets a specific velocity value', () => {
    const store = usePatternsStore()
    store.addStyle('test')
    store.setVelocity(0, 0, 0.7)
    expect(store.styles.get('test')!.grid[0][0]).toBe(0.7)
  })

  it('activePattern reflects the active style', () => {
    const store = usePatternsStore()
    store.addStyle('pagode')
    store.setVelocity(3, 7, 1.0)
    expect(store.activePattern!.grid[3][7]).toBe(1.0)

    store.addStyle('frevo')
    // frevo is now active, should have empty grid
    expect(store.activePattern!.grid[3][7]).toBe(0)
  })

  it('exportJson produces valid JSON with all styles', () => {
    const store = usePatternsStore()
    store.addStyle('samba')
    store.addStyle('frevo')

    const json = store.exportJson()
    const parsed = JSON.parse(json)
    expect(parsed.samba).toBeDefined()
    expect(parsed.frevo).toBeDefined()
    expect(Array.isArray(parsed.samba)).toBe(true)
    expect(parsed.samba.length).toBe(14)
  })

  it('importJson loads patterns from JSON', () => {
    const store = usePatternsStore()
    const grid = Array.from({ length: 14 }, () => Array(16).fill(0.5))
    const json = JSON.stringify({ importado: grid })

    store.importJson(json)
    expect(store.styles.get('importado')!.grid[0][0]).toBe(0.5)
  })

  describe('undo/redo', () => {
    it('starts with no undo history', () => {
      const store = usePatternsStore()
      expect(store.canUndo).toBe(false)
      expect(store.canRedo).toBe(false)
    })

    it('pops undo after setVelocity', () => {
      const store = usePatternsStore()
      store.addStyle('test')
      expect(store.canUndo).toBe(false)

      store.setVelocity(0, 0, 0.7)
      expect(store.canUndo).toBe(true)
    })

    it('undo restores previous velocity', () => {
      const store = usePatternsStore()
      store.addStyle('test')
      store.setVelocity(0, 0, 0.7)
      expect(store.activePattern!.grid[0][0]).toBe(0.7)

      store.undo()
      expect(store.activePattern!.grid[0][0]).toBe(0)
    })

    it('redo restores the undone change', () => {
      const store = usePatternsStore()
      store.addStyle('test')
      store.setVelocity(0, 0, 0.7)
      store.undo()
      expect(store.activePattern!.grid[0][0]).toBe(0)

      store.redo()
      expect(store.activePattern!.grid[0][0]).toBe(0.7)
    })

    it('multiple changes tracked independently', () => {
      const store = usePatternsStore()
      store.addStyle('test')
      store.setVelocity(0, 0, 0.4)
      store.setVelocity(1, 1, 0.7)
      store.setVelocity(2, 2, 1.0)

      store.undo()
      expect(store.activePattern!.grid[2][2]).toBe(0)
      store.undo()
      expect(store.activePattern!.grid[1][1]).toBe(0)
      store.undo()
      expect(store.activePattern!.grid[0][0]).toBe(0)
      expect(store.canUndo).toBe(false)
    })

    it('new edit after undo discards redo stack', () => {
      const store = usePatternsStore()
      store.addStyle('test')
      store.setVelocity(0, 0, 0.4)
      store.setVelocity(0, 0, 0.7)
      store.undo()  // back to 0.4
      expect(store.canRedo).toBe(true)

      store.setVelocity(0, 0, 1.0)  // new edit — discards redo
      expect(store.canRedo).toBe(false)
      store.undo()
      expect(store.activePattern!.grid[0][0]).toBe(0.4)
    })

    it('cycleVelocity pushes undo', () => {
      const store = usePatternsStore()
      store.addStyle('test')
      store.cycleVelocity(0, 0)
      expect(store.activePattern!.grid[0][0]).toBe(0.4)
      store.undo()
      expect(store.activePattern!.grid[0][0]).toBe(0)
    })

    it('canUndo/canRedo update correctly at boundaries', () => {
      const store = usePatternsStore()
      store.addStyle('test')
      expect(store.canUndo).toBe(false)
      expect(store.canRedo).toBe(false)

      store.setVelocity(0, 0, 0.5)
      expect(store.canUndo).toBe(true)
      expect(store.canRedo).toBe(false)

      store.undo()
      expect(store.canUndo).toBe(false)
      expect(store.canRedo).toBe(true)

      store.redo()
      expect(store.canUndo).toBe(true)
      expect(store.canRedo).toBe(false)
    })
  })

  describe('copy/paste/clear track', () => {
    it('copyTrack clones the track data', () => {
      const store = usePatternsStore()
      store.addStyle('test')
      store.setVelocity(0, 0, 0.7)
      store.setVelocity(0, 5, 1.0)

      store.copyTrack(0)
      expect(store.clipTrackGrid).not.toBeNull()
      expect(store.clipTrackGrid).toHaveLength(16)
      expect(store.clipTrackGrid![0]).toBe(0.7)
      expect(store.clipTrackGrid![5]).toBe(1.0)
    })

    it('pasteTrack copies clipboard to target track', () => {
      const store = usePatternsStore()
      store.addStyle('test')
      store.setVelocity(0, 0, 0.9)
      store.copyTrack(0)

      // Modify original track to verify independence
      store.setVelocity(0, 0, 0.1)

      store.pasteTrack(5)
      expect(store.activePattern!.grid[5][0]).toBe(0.9) // pasted value
      expect(store.activePattern!.grid[0][0]).toBe(0.1) // original changed
    })

    it('clearTrack zeros the track', () => {
      const store = usePatternsStore()
      store.addStyle('test')
      store.setVelocity(0, 0, 1.0)
      store.setVelocity(0, 15, 1.0)

      store.clearTrack(0)
      expect(store.activePattern!.grid[0][0]).toBe(0)
      expect(store.activePattern!.grid[0][15]).toBe(0)
    })

    it('pasteTrack is no-op when clipboard is empty', () => {
      const store = usePatternsStore()
      store.addStyle('test')
      store.setVelocity(0, 0, 1.0)
      expect(store.clipTrackGrid).toBeNull()

      store.pasteTrack(0)  // should be no-op
      expect(store.activePattern!.grid[0][0]).toBe(1.0) // unchanged
    })

    it('clipboard persists across undo/redo', () => {
      const store = usePatternsStore()
      store.addStyle('test')
      store.setVelocity(0, 0, 0.5)
      store.copyTrack(0)
      store.undo()
      store.redo()
      expect(store.clipTrackGrid![0]).toBe(0.5) // clipboard survives
    })
  })

  describe('setRange', () => {
    it('fills a rectangular area with a velocity', () => {
      const store = usePatternsStore()
      store.addStyle('test')
      store.setRange(0, 3, 0, 3, 0.8)
      // corners
      expect(store.activePattern!.grid[0][0]).toBe(0.8)
      expect(store.activePattern!.grid[3][3]).toBe(0.8)
      // outside range
      expect(store.activePattern!.grid[4][0]).toBe(0)
      expect(store.activePattern!.grid[0][4]).toBe(0)
    })

    it('respects grid bounds', () => {
      const store = usePatternsStore()
      store.addStyle('test')
      store.setRange(-1, 20, -1, 20, 0.5)  // out of bounds
      // Should not crash, only fill valid range (0-13, 0-15)
      expect(store.activePattern!.grid[0][0]).toBe(0.5)
      expect(store.activePattern!.grid[13][15]).toBe(0.5)
    })
  })

  it('loadDefaults populates styles with built-in patterns', () => {
    const store = usePatternsStore()
    store.loadDefaults()

    expect(store.styles.has('samba')).toBe(true)
    expect(store.styles.has('pagode')).toBe(true)
    expect(store.styles.has('frevo')).toBe(true)
    expect(store.styles.has('maracatu')).toBe(true)
    expect(store.styles.has('ijexa')).toBe(true)
    expect(store.styles.has('samba_reggae')).toBe(true)
    expect(store.styles.has('partido_alto')).toBe(true)
    expect(store.styles.has('intro')).toBe(true)
    expect(store.styles.has('virada')).toBe(true)
    expect(store.activeStyle).toBe('samba')
  })

  it('loadDefaults does not overwrite existing user patterns', () => {
    const store = usePatternsStore()
    store.addStyle('samba')
    store.setVelocity(5, 5, 0.99)

    store.loadDefaults()
    // User modifications should be preserved
    expect(store.styles.get('samba')!.grid[5][5]).toBe(0.99)
  })
})
