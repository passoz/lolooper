/**
 * Unit tests for SongEditorPage logic (store-based, no DOM rendering).
 */
import { describe, it, expect, beforeEach, vi } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useSongsStore } from '../stores/songs'

vi.mock('../db/database', () => ({
  db: {
    loadSongs: vi.fn().mockResolvedValue([]),
    saveSongs: vi.fn().mockResolvedValue(undefined),
  },
}))

describe('SongEditorPage — store logic', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('creates a song with default fields', () => {
    const store = useSongsStore()
    store.addSong({
      id: crypto.randomUUID(),
      name: 'Samba de Roda',
      bpm: 104,
      key: 'Dm',
      style: 'samba',
      sections: [],
    })
    expect(store.songs).toHaveLength(1)
    expect(store.songs[0].name).toBe('Samba de Roda')
    expect(store.songs[0].bpm).toBe(104)
    expect(store.songs[0].key).toBe('Dm')
  })

  it('adds and removes sections', () => {
    const store = useSongsStore()
    const id = crypto.randomUUID()
    store.addSong({ id, name: 'Test', bpm: 100, key: 'C', style: 'samba', sections: [] })

    // Add sections
    store.updateSong(id, {
      sections: [
        { name: 'intro', patternStyle: 'intro', bars: 2, actions: [] },
        { name: 'refrao', patternStyle: 'samba', bars: 8, actions: [] },
      ],
    })
    expect(store.songs[0].sections).toHaveLength(2)

    // Remove a section (simulate SongEditorPage behavior)
    const filtered = store.songs[0].sections.filter(s => s.name !== 'intro')
    store.updateSong(id, { sections: filtered })
    expect(store.songs[0].sections).toHaveLength(1)
    expect(store.songs[0].sections[0].name).toBe('refrao')
  })

  it('moves sections by reordering', () => {
    const store = useSongsStore()
    const id = crypto.randomUUID()
    store.addSong({ id, name: 'Test', bpm: 100, key: 'C', style: 'samba', sections: [] })

    const sections = [
      { name: 'A', patternStyle: 'samba', bars: 4, actions: [] },
      { name: 'B', patternStyle: 'pagode', bars: 8, actions: [] },
      { name: 'C', patternStyle: 'frevo', bars: 2, actions: [] },
    ]
    store.updateSong(id, { sections })

    // Simulate moveSection: remove from index 0, insert at index 2
    const [removed] = store.songs[0].sections.splice(0, 1)
    store.songs[0].sections.splice(2, 0, removed)
    const reordered = [...store.songs[0].sections]
    store.updateSong(id, { sections: reordered })

    expect(store.songs[0].sections[0].name).toBe('B')
    expect(store.songs[0].sections[2].name).toBe('A')
  })

  it('adds and removes actions from sections', () => {
    const store = useSongsStore()
    const id = crypto.randomUUID()
    store.addSong({ id, name: 'Test', bpm: 100, key: 'C', style: 'samba', sections: [] })

    // Add section with an action
    store.updateSong(id, {
      sections: [{
        name: 'verso', patternStyle: 'samba', bars: 16,
        actions: [{ atBar: 0, cmd: 'mute', track: 'cavaquinho' }],
      }],
    })
    expect(store.songs[0].sections[0].actions).toHaveLength(1)

    // Add another action
    const sections = store.songs[0].sections.map(s => ({
      ...s,
      actions: [...s.actions, { atBar: 4, cmd: 'unmute', track: 'cavaquinho' }],
    }))
    store.updateSong(id, { sections })
    expect(store.songs[0].sections[0].actions).toHaveLength(2)

    // Remove the first action
    const sections2 = store.songs[0].sections.map(s => ({
      ...s,
      actions: s.actions.filter((_, i) => i !== 0),
    }))
    store.updateSong(id, { sections: sections2 })
    expect(store.songs[0].sections[0].actions).toHaveLength(1)
    expect(store.songs[0].sections[0].actions[0].cmd).toBe('unmute')
  })

  it('exports song as JSON and re-imports it', () => {
    const store = useSongsStore()
    const id = crypto.randomUUID()
    store.addSong({
      id, name: 'Export Test', bpm: 120, key: 'Am', style: 'samba',
      sections: [{ name: 'A', patternStyle: 'samba', bars: 8, actions: [] }],
    })

    // Simulate export (SongEditorPage exportSong)
    const json = JSON.stringify(store.songs[0], null, 2)
    const parsed = JSON.parse(json)

    // Simulate re-import with new ID
    parsed.id = crypto.randomUUID()
    store.addSong(parsed)
    expect(store.songs).toHaveLength(2)
    expect(store.songs[1].name).toBe('Export Test')
    expect(store.songs[1].bpm).toBe(120)
  })
})
