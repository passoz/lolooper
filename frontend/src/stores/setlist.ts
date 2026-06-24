import { defineStore } from 'pinia'
import { ref } from 'vue'
import { db } from '../db/database'

export interface Setlist {
  id: string
  name: string
  date: string
  songs: string[]
}

export const useSetlistStore = defineStore('setlist', () => {
  const setlists = ref<Setlist[]>([])
  const activeSetlist = ref<Setlist | null>(null)

  function createSetlist(name: string) {
    const setlist: Setlist = {
      id: crypto.randomUUID(),
      name,
      date: new Date().toISOString().split('T')[0],
      songs: [],
    }
    setlists.value.push(setlist)
    db.saveSetlists(setlists.value)
    return setlist
  }

  function addSongToSetlist(setlistId: string, songId: string) {
    const setlist = setlists.value.find(s => s.id === setlistId)
    if (setlist) {
      setlist.songs.push(songId)
      db.saveSetlists(setlists.value)
    }
  }

  function removeSongFromSetlist(setlistId: string, songId: string) {
    const setlist = setlists.value.find(s => s.id === setlistId)
    if (setlist) {
      setlist.songs = setlist.songs.filter(id => id !== songId)
      db.saveSetlists(setlists.value)
    }
  }

  function reorder(setlistId: string, from: number, to: number) {
    const setlist = setlists.value.find(s => s.id === setlistId)
    if (!setlist) return
    const [removed] = setlist.songs.splice(from, 1)
    setlist.songs.splice(to, 0, removed)
    db.saveSetlists(setlists.value)
  }

  async function loadFromDB() {
    const saved = await db.loadSetlists()
    if (saved) setlists.value = saved
  }

  return {
    setlists,
    activeSetlist,
    createSetlist,
    addSongToSetlist,
    removeSongFromSetlist,
    reorder,
    loadFromDB,
  }
})
