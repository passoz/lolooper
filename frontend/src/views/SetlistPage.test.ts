/**
 * Unit tests for SetlistPage logic (store-based, no DOM rendering).
 */
import { describe, it, expect, beforeEach, vi } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useSetlistStore } from '../stores/setlist'
import { useSongsStore } from '../stores/songs'

vi.mock('../db/database', () => ({
  db: {
    loadSetlists: vi.fn().mockResolvedValue([]),
    saveSetlists: vi.fn().mockResolvedValue(undefined),
    loadSongs: vi.fn().mockResolvedValue([]),
    saveSongs: vi.fn().mockResolvedValue(undefined),
  },
}))

describe('SetlistPage — store logic', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('creates a setlist and adds songs from the songs store', () => {
    const setlistStore = useSetlistStore()
    const songsStore = useSongsStore()

    // Create songs
    const song1Id = crypto.randomUUID()
    const song2Id = crypto.randomUUID()
    songsStore.addSong({ id: song1Id, name: 'Samba A', bpm: 100, key: 'Cm', style: 'samba', sections: [] })
    songsStore.addSong({ id: song2Id, name: 'Pagode B', bpm: 120, key: 'Dm', style: 'pagode', sections: [] })

    // Create setlist
    const sl = setlistStore.createSetlist('Show 15/07')
    expect(sl.name).toBe('Show 15/07')
    expect(sl.songs).toHaveLength(0)

    // Add songs
    setlistStore.addSongToSetlist(sl.id, song1Id)
    setlistStore.addSongToSetlist(sl.id, song2Id)
    expect(setlistStore.setlists[0].songs).toHaveLength(2)
    expect(setlistStore.setlists[0].songs[0]).toBe(song1Id)
    expect(setlistStore.setlists[0].songs[1]).toBe(song2Id)
  })

  it('availableSongs filters songs already in the setlist', () => {
    const setlistStore = useSetlistStore()
    const songsStore = useSongsStore()

    const s1 = crypto.randomUUID()
    const s2 = crypto.randomUUID()
    const s3 = crypto.randomUUID()
    songsStore.addSong({ id: s1, name: 'A', bpm: 100, key: 'C', style: 'samba', sections: [] })
    songsStore.addSong({ id: s2, name: 'B', bpm: 100, key: 'C', style: 'samba', sections: [] })
    songsStore.addSong({ id: s3, name: 'C', bpm: 100, key: 'C', style: 'samba', sections: [] })

    const sl = setlistStore.createSetlist('Test')
    setlistStore.addSongToSetlist(sl.id, s1)
    setlistStore.addSongToSetlist(sl.id, s2)

    // Available = songs not in setlist = [s3]
    const idsInSetlist = new Set(setlistStore.setlists[0].songs)
    const available = songsStore.songs.filter(s => !idsInSetlist.has(s.id))
    expect(available).toHaveLength(1)
    expect(available[0].id).toBe(s3)
  })

  it('moves a song to the top of the setlist', () => {
    const setlistStore = useSetlistStore()
    const sl = setlistStore.createSetlist('Test')
    setlistStore.addSongToSetlist(sl.id, 'a')
    setlistStore.addSongToSetlist(sl.id, 'b')
    setlistStore.addSongToSetlist(sl.id, 'c')

    // Move 'c' to position 0
    setlistStore.reorder(sl.id, 2, 0)
    expect(setlistStore.setlists[0].songs).toEqual(['c', 'a', 'b'])
  })

  it('removeSongFromSetlist removes and preserves order', () => {
    const setlistStore = useSetlistStore()
    const sl = setlistStore.createSetlist('Test')
    setlistStore.addSongToSetlist(sl.id, 'x')
    setlistStore.addSongToSetlist(sl.id, 'y')
    setlistStore.addSongToSetlist(sl.id, 'z')

    setlistStore.removeSongFromSetlist(sl.id, 'x')
    expect(setlistStore.setlists[0].songs).toEqual(['y', 'z'])
  })

  it('can add song to empty setlist', () => {
    const setlistStore = useSetlistStore()
    const sl = setlistStore.createSetlist('Vazio')
    setlistStore.addSongToSetlist(sl.id, 'primeira')
    expect(setlistStore.setlists[0].songs).toEqual(['primeira'])
  })

  it('tracks song count in setlist summary', () => {
    const setlistStore = useSetlistStore()
    const sl = setlistStore.createSetlist('Show')
    expect(setlistStore.setlists[0].songs.length).toBe(0)
    setlistStore.addSongToSetlist(sl.id, 's1')
    setlistStore.addSongToSetlist(sl.id, 's2')
    expect(setlistStore.setlists[0].songs.length).toBe(2)
  })
})
