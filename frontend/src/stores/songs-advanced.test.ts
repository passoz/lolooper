/**
 * Advanced tests for the songs Pinia store: sections and actions.
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

describe('useSongsStore — Sections & Actions', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  describe('section management', () => {
    it('can add a section to a song', () => {
      const store = useSongsStore()
      const song = makeSong()
      store.addSong(song)

      store.updateSong(song.id, {
        sections: [{ name: 'intro', patternStyle: 'intro', bars: 4, actions: [] }],
      })

      expect(store.songs[0].sections).toHaveLength(1)
      expect(store.songs[0].sections[0].name).toBe('intro')
      expect(store.songs[0].sections[0].bars).toBe(4)
    })

    it('can reorder sections', () => {
      const store = useSongsStore()
      const song = makeSong()
      store.addSong(song)

      const sections = [
        { name: 'A', patternStyle: 'samba', bars: 4, actions: [] },
        { name: 'B', patternStyle: 'pagode', bars: 8, actions: [] },
        { name: 'C', patternStyle: 'frevo', bars: 2, actions: [] },
      ]
      store.updateSong(song.id, { sections })

      // Move A (index 0) to end (index 2)
      const [removed] = (store.songs[0].sections as typeof sections).splice(0, 1)
      ;(store.songs[0].sections as typeof sections).splice(2, 0, removed)

      expect(store.songs[0].sections[0].name).toBe('B')
      expect(store.songs[0].sections[1].name).toBe('C')
      expect(store.songs[0].sections[2].name).toBe('A')
    })

    it('can remove a section', () => {
      const store = useSongsStore()
      const song = makeSong()
      store.addSong(song)

      store.updateSong(song.id, {
        sections: [
          { name: 'A', patternStyle: 'samba', bars: 4, actions: [] },
          { name: 'B', patternStyle: 'samba', bars: 8, actions: [] },
        ],
      })

      store.updateSong(song.id, {
        sections: store.songs[0].sections.filter(s => s.name !== 'A'),
      })

      expect(store.songs[0].sections).toHaveLength(1)
      expect(store.songs[0].sections[0].name).toBe('B')
    })
  })

  describe('section actions', () => {
    it('can add actions to a section', () => {
      const store = useSongsStore()
      const song = makeSong()
      store.addSong(song)

      store.updateSong(song.id, {
        sections: [{
          name: 'refrao',
          patternStyle: 'samba',
          bars: 16,
          actions: [{ atBar: 0, cmd: 'mute', track: 'cavaquinho' }],
        }],
      })

      expect(store.songs[0].sections[0].actions).toHaveLength(1)
      expect(store.songs[0].sections[0].actions[0].cmd).toBe('mute')
      expect(store.songs[0].sections[0].actions[0].track).toBe('cavaquinho')
    })

    it('can have multiple actions on one section', () => {
      const store = useSongsStore()
      const song = makeSong()
      store.addSong(song)

      store.updateSong(song.id, {
        sections: [{
          name: 'verso',
          patternStyle: 'samba',
          bars: 16,
          actions: [
            { atBar: 0, cmd: 'mute', track: 'caixa' },
            { atBar: 4, cmd: 'set_accent', track: 'pandeiro', value: 1.5 },
            { atBar: 12, cmd: 'fade_out', duration: 4 },
          ],
        }],
      })

      expect(store.songs[0].sections[0].actions).toHaveLength(3)
      expect(store.songs[0].sections[0].actions[1].cmd).toBe('set_accent')
      expect(store.songs[0].sections[0].actions[1].value).toBe(1.5)
    })

    it('can remove an action from a section', () => {
      const store = useSongsStore()
      const song = makeSong()
      store.addSong(song)

      const actions = [
        { atBar: 0, cmd: 'mute' as const, track: 'caixa' },
        { atBar: 4, cmd: 'unmute' as const, track: 'caixa' },
      ]
      store.updateSong(song.id, {
        sections: [{ name: 'A', patternStyle: 'samba', bars: 8, actions }],
      })

      store.updateSong(song.id, {
        sections: [{
          ...store.songs[0].sections[0],
          actions: store.songs[0].sections[0].actions.filter((_, i) => i !== 0),
        }],
      })

      expect(store.songs[0].sections[0].actions).toHaveLength(1)
      expect(store.songs[0].sections[0].actions[0].cmd).toBe('unmute')
    })
  })

  describe('song serialization for SysEx', () => {
    it('produces valid song structure JSON', () => {
      const store = useSongsStore()
      const song = makeSong()
      store.addSong(song)

      store.updateSong(song.id, {
        sections: [
          { name: 'intro', patternStyle: 'intro', bars: 2, actions: [] },
          { name: 'refrao', patternStyle: 'samba', bars: 8, actions: [
            { atBar: 0, cmd: 'mute', track: 'cavaquinho' },
          ]},
        ],
      })

      // Simulate the SysEx serialization pattern from SongEditorPage
      const data = {
        name: store.songs[0].name,
        bpm: store.songs[0].bpm,
        sections: store.songs[0].sections.map(s => ({
          name: s.name,
          pattern: s.patternStyle,
          bars: s.bars,
          loop: false,
        })),
      }

      const json = JSON.stringify(data)
      const parsed = JSON.parse(json)

      expect(parsed.name).toBe('Test Song')
      expect(parsed.sections).toHaveLength(2)
      expect(parsed.sections[0].name).toBe('intro')
      expect(parsed.sections[0].pattern).toBe('intro')
      expect(parsed.sections[0].bars).toBe(2)
      expect(parsed.sections[1].pattern).toBe('samba')
      expect(parsed.sections[1].bars).toBe(8)
    })
  })
})
