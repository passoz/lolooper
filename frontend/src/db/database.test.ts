/**
 * Unit tests for the database module.
 * Mocks idb-keyval to test the CRUD wrapper without real IndexedDB.
 */
import { describe, it, expect, beforeEach, vi } from 'vitest'

// Mock idb-keyval
const mockStore: Record<string, unknown> = {}
vi.mock('idb-keyval', () => ({
  get: vi.fn((key: string) => Promise.resolve(mockStore[key] ?? undefined)),
  set: vi.fn((key: string, value: unknown) => {
    mockStore[key] = value
    return Promise.resolve()
  }),
  del: vi.fn((key: string) => {
    delete mockStore[key]
    return Promise.resolve()
  }),
  keys: vi.fn(() => Promise.resolve(Object.keys(mockStore))),
}))

import { db } from './database'

describe('db (database)', () => {
  beforeEach(() => {
    Object.keys(mockStore).forEach(k => delete mockStore[k])
  })

  describe('patterns', () => {
    it('loadPatterns returns null when no data', async () => {
      const result = await db.loadPatterns()
      expect(result).toBeNull()
    })

    it('savePatterns and loadPatterns round-trip', async () => {
      const data = { samba: [[0, 0.4, 0.7, 1.0]] }
      await db.savePatterns(data)
      const loaded = await db.loadPatterns()
      expect(loaded).toEqual(data)
    })
  })

  describe('songs', () => {
    it('loadSongs returns empty array by default', async () => {
      const result = await db.loadSongs()
      expect(result).toEqual([])
    })

    it('saveSongs and loadSongs round-trip', async () => {
      const songs = [{ id: '1', name: 'Samba de Roda', bpm: 104, key: 'Dm', style: 'samba', sections: [] }]
      await db.saveSongs(songs)
      const loaded = await db.loadSongs()
      expect(loaded).toEqual(songs)
    })
  })

  describe('setlists', () => {
    it('round-trip saves and loads setlists', async () => {
      const setlists = [{ id: '1', name: 'Show', date: '2026-07-15', songs: ['song-1'] }]
      await db.saveSetlists(setlists)
      const loaded = await db.loadSetlists()
      expect(loaded).toEqual(setlists)
    })
  })

  describe('settings', () => {
    it('loadSettings returns defaults when no data', async () => {
      const settings = await db.loadSettings()
      expect(settings.theme).toBe('dark')
      expect(settings.iaMode).toBe('off')
      expect(settings.midiOutputId).toBeNull()
    })

    it('saveSettings and loadSettings round-trip', async () => {
      await db.saveSettings({ midiOutputId: 'port-1', iaMode: 'assist', theme: 'dark' })
      const loaded = await db.loadSettings()
      expect(loaded.midiOutputId).toBe('port-1')
      expect(loaded.iaMode).toBe('assist')
    })
  })

  describe('clearAll', () => {
    it('removes all keys', async () => {
      await db.savePatterns({ samba: [[]] })
      await db.saveSongs([{ id: '1' }])
      await db.clearAll()
      expect(await db.loadPatterns()).toBeNull()
      expect(await db.loadSongs()).toEqual([])
    })
  })
})
