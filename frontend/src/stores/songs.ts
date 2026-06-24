import { defineStore } from 'pinia'
import { ref } from 'vue'
import { db } from '../db/database'

export interface Section {
  name: string
  patternStyle: string
  bars: number
  actions: SectionAction[]
}

export interface SectionAction {
  atBar: number
  cmd: string
  track?: string
  value?: number
  style?: string
  duration?: number
}

export interface Song {
  id: string
  name: string
  bpm: number
  key: string
  style: string
  sections: Section[]
}

export const useSongsStore = defineStore('songs', () => {
  const songs = ref<Song[]>([])
  const currentSong = ref<Song | null>(null)

  function addSong(song: Song) {
    songs.value.push(song)
    db.saveSongs(songs.value)
  }

  function removeSong(id: string) {
    songs.value = songs.value.filter(s => s.id !== id)
    db.saveSongs(songs.value)
  }

  function updateSong(id: string, data: Partial<Song>) {
    const idx = songs.value.findIndex(s => s.id === id)
    if (idx !== -1) {
      songs.value[idx] = { ...songs.value[idx], ...data }
      db.saveSongs(songs.value)
    }
  }

  async function loadFromDB() {
    const saved = await db.loadSongs()
    if (saved) songs.value = saved
  }

  return {
    songs,
    currentSong,
    addSong,
    removeSong,
    updateSong,
    loadFromDB,
  }
})
