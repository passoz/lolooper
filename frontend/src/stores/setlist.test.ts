/**
 * Unit tests for the setlist Pinia store.
 */
import { describe, it, expect, beforeEach, vi } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useSetlistStore } from './setlist'

vi.mock('../db/database', () => ({
  db: {
    loadSetlists: vi.fn().mockResolvedValue([]),
    saveSetlists: vi.fn().mockResolvedValue(undefined),
  },
}))

describe('useSetlistStore', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('initializes with empty setlists array', () => {
    const store = useSetlistStore()
    expect(store.setlists).toEqual([])
    expect(store.activeSetlist).toBeNull()
  })

  it('createSetlist creates with name and today date', () => {
    const store = useSetlistStore()
    const result = store.createSetlist('Show JUL/2026')

    expect(result.name).toBe('Show JUL/2026')
    expect(result.songs).toEqual([])
    expect(result.id).toBeDefined()
    // Date format: YYYY-MM-DD
    expect(result.date).toMatch(/^\d{4}-\d{2}-\d{2}$/)
    expect(store.setlists).toHaveLength(1)
  })

  it('addSongToSetlist adds a song ID', () => {
    const store = useSetlistStore()
    const sl = store.createSetlist('Show')
    store.addSongToSetlist(sl.id, 'song-1')
    store.addSongToSetlist(sl.id, 'song-2')

    expect(store.setlists[0].songs).toEqual(['song-1', 'song-2'])
  })

  it('addSongToSetlist is no-op for unknown setlist', () => {
    const store = useSetlistStore()
    store.addSongToSetlist('ghost-id', 'song-1')
    expect(store.setlists).toHaveLength(0)
  })

  it('removeSongFromSetlist removes by ID', () => {
    const store = useSetlistStore()
    const sl = store.createSetlist('Show')
    store.addSongToSetlist(sl.id, 'song-1')
    store.addSongToSetlist(sl.id, 'song-2')
    store.addSongToSetlist(sl.id, 'song-3')

    store.removeSongFromSetlist(sl.id, 'song-2')
    expect(store.setlists[0].songs).toEqual(['song-1', 'song-3'])
  })

  it('reorder moves a song within the setlist', () => {
    const store = useSetlistStore()
    const sl = store.createSetlist('Show')
    store.addSongToSetlist(sl.id, 'song-A')
    store.addSongToSetlist(sl.id, 'song-B')
    store.addSongToSetlist(sl.id, 'song-C')

    store.reorder(sl.id, 0, 2) // move first to end
    expect(store.setlists[0].songs).toEqual(['song-B', 'song-C', 'song-A'])
  })

  it('reorder is no-op for unknown setlist', () => {
    const store = useSetlistStore()
    const sl = store.createSetlist('Show')
    store.addSongToSetlist(sl.id, 'song-A')

    store.reorder('ghost-id', 0, 1)
    expect(store.setlists[0].songs).toEqual(['song-A'])
  })

  describe('songs management', () => {
    it('adds multiple songs to a setlist', () => {
      const store = useSetlistStore()
      const sl = store.createSetlist('Set A')
      store.addSongToSetlist(sl.id, 's1')
      store.addSongToSetlist(sl.id, 's2')
      store.addSongToSetlist(sl.id, 's3')

      expect(store.setlists[0].songs).toHaveLength(3)
      expect(store.setlists[0].songs).toEqual(['s1', 's2', 's3'])
    })

    it('removes song preserves order of remaining', () => {
      const store = useSetlistStore()
      const sl = store.createSetlist('Set A')
      store.addSongToSetlist(sl.id, 'a')
      store.addSongToSetlist(sl.id, 'b')
      store.addSongToSetlist(sl.id, 'c')

      store.removeSongFromSetlist(sl.id, 'b')
      expect(store.setlists[0].songs).toEqual(['a', 'c'])
    })

    it('removes first song', () => {
      const store = useSetlistStore()
      const sl = store.createSetlist('Set A')
      store.addSongToSetlist(sl.id, 'first')
      store.addSongToSetlist(sl.id, 'second')
      store.removeSongFromSetlist(sl.id, 'first')
      expect(store.setlists[0].songs).toEqual(['second'])
    })

    it('reorder moves song up', () => {
      const store = useSetlistStore()
      const sl = store.createSetlist('Set A')
      store.addSongToSetlist(sl.id, 'A')
      store.addSongToSetlist(sl.id, 'B')
      store.addSongToSetlist(sl.id, 'C')

      store.reorder(sl.id, 2, 0) // move C to front
      expect(store.setlists[0].songs).toEqual(['C', 'A', 'B'])
    })

    it('empty setlist', () => {
      const store = useSetlistStore()
      const sl = store.createSetlist('Empty')
      expect(sl.songs).toEqual([])
    })
  })
})
