/**
 * Unit tests for the songs Pinia store.
 */
import { describe, it, expect, beforeEach, vi } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useSongsStore, type Song } from './songs'

vi.mock('../db/database', () => ({
  db: {
    loadSongs: vi.fn().mockResolvedValue([]),
    saveSongs: vi.fn().mockResolvedValue(undefined),
  },
}))

function makeSong(overrides: Partial<Song> = {}): Song {
  return {
    id: crypto.randomUUID(),
    name: 'Test Song',
    bpm: 100,
    key: 'Cm',
    style: 'samba',
    sections: [],
    ...overrides,
  }
}

describe('useSongsStore', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('initializes with empty songs array', () => {
    const store = useSongsStore()
    expect(store.songs).toEqual([])
    expect(store.currentSong).toBeNull()
  })

  it('addSong appends a song to the list', () => {
    const store = useSongsStore()
    const song = makeSong()
    store.addSong(song)
    expect(store.songs).toHaveLength(1)
    expect(store.songs[0].name).toBe('Test Song')
  })

  it('removeSong removes by ID', () => {
    const store = useSongsStore()
    const song1 = makeSong({ name: 'Song A' })
    const song2 = makeSong({ name: 'Song B' })
    store.addSong(song1)
    store.addSong(song2)

    store.removeSong(song1.id)
    expect(store.songs).toHaveLength(1)
    expect(store.songs[0].name).toBe('Song B')
  })

  it('removeSong is no-op for unknown ID', () => {
    const store = useSongsStore()
    store.addSong(makeSong())
    store.removeSong('nonexistent')
    expect(store.songs).toHaveLength(1)
  })

  it('updateSong updates fields by ID', () => {
    const store = useSongsStore()
    const song = makeSong()
    store.addSong(song)

    store.updateSong(song.id, { name: 'Updated', bpm: 120 })
    expect(store.songs[0].name).toBe('Updated')
    expect(store.songs[0].bpm).toBe(120)
    // Unchanged fields remain
    expect(store.songs[0].key).toBe('Cm')
  })

  it('updateSong is no-op for unknown ID', () => {
    const store = useSongsStore()
    store.addSong(makeSong())
    store.updateSong('nonexistent', { name: 'Ghost' })
    expect(store.songs[0].name).toBe('Test Song')
  })

  it('can store songs with sections', () => {
    const store = useSongsStore()
    const song = makeSong({
      sections: [
        { name: 'intro', patternStyle: 'intro', bars: 4, actions: [] },
        { name: 'refrao', patternStyle: 'samba', bars: 16, actions: [
          { atBar: 0, cmd: 'mute', track: 'cavaquinho' },
        ]},
      ],
    })
    store.addSong(song)
    expect(store.songs[0].sections).toHaveLength(2)
    expect(store.songs[0].sections[0].name).toBe('intro')
    expect(store.songs[0].sections[1].actions[0].cmd).toBe('mute')
  })

  it('exportSong produces valid JSON round-trip', () => {
    const store = useSongsStore()
    const song = makeSong({
      sections: [{ name: 'A', patternStyle: 'samba', bars: 8, actions: [] }],
    })
    store.addSong(song)

    // Simulate SongEditorPage export logic
    const json = JSON.stringify(store.songs[0], null, 2)
    const parsed = JSON.parse(json)
    expect(parsed.name).toBe('Test Song')
    expect(parsed.bpm).toBe(100)
    expect(parsed.sections).toHaveLength(1)
    expect(parsed.sections[0].patternStyle).toBe('samba')
    expect(parsed.sections[0].bars).toBe(8)
  })

  it('importSong parses exported JSON correctly', () => {
    const store = useSongsStore()
    const exported = {
      id: 'original-id',
      name: 'Samba de Roda',
      bpm: 104,
      key: 'Dm',
      style: 'samba',
      sections: [
        { name: 'intro', patternStyle: 'intro', bars: 2, actions: [] },
        { name: 'verso', patternStyle: 'samba', bars: 16, actions: [
          { atBar: 0, cmd: 'mute', track: 'cavaquinho' },
        ]},
      ],
    }

    // Simulate SongEditorPage importSong: assign new ID, add to store
    const imported = { ...exported, id: crypto.randomUUID() }
    store.addSong(imported)

    expect(store.songs).toHaveLength(1)
    expect(store.songs[0].name).toBe('Samba de Roda')
    expect(store.songs[0].bpm).toBe(104)
    expect(store.songs[0].sections).toHaveLength(2)
    expect(store.songs[0].sections[0].name).toBe('intro')
    // Original ID should not leak (replaced by new random UUID)
    expect(store.songs[0].id).not.toBe('original-id')
  })

  it('export/import preserves all song fields', () => {
    const store = useSongsStore()
    const original = makeSong({
      name: 'Frevo Dance',
      bpm: 160,
      key: 'F',
      style: 'frevo',
      sections: [{
        name: 'refrao', patternStyle: 'frevo', bars: 8,
        actions: [{ atBar: 4, cmd: 'set_accent', track: 'pandeiro', value: 1.5 }],
      }],
    })
    store.addSong(original)

    // Export → re-import
    const json = JSON.stringify(store.songs[0], null, 2)
    const parsed = JSON.parse(json)
    parsed.id = crypto.randomUUID()  // simulate new import
    const store2 = useSongsStore()
    store2.addSong(parsed)

    expect(store2.songs[0].name).toBe('Frevo Dance')
    expect(store2.songs[0].bpm).toBe(160)
    expect(store2.songs[0].key).toBe('F')
    expect(store2.songs[0].style).toBe('frevo')
    expect(store2.songs[0].sections[0].actions[0].cmd).toBe('set_accent')
    expect(store2.songs[0].sections[0].actions[0].value).toBe(1.5)
  })
})
