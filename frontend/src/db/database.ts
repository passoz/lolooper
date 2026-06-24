import { get, set, del, keys } from 'idb-keyval'

export interface Settings {
  midiOutputId: string | null
  iaMode: 'off' | 'assist' | 'watch' | 'full'
  theme: 'dark' | 'light'
}

const defaultSettings: Settings = {
  midiOutputId: null,
  iaMode: 'off',
  theme: 'dark',
}

export const db = {
  // Patterns
  async loadPatterns(): Promise<Record<string, number[][]> | null> {
    return (await get('patterns')) || null
  },
  async savePatterns(data: Record<string, number[][]>): Promise<void> {
    await set('patterns', data)
  },

  // Songs
  async loadSongs() {
    return (await get('songs')) || []
  },
  async saveSongs(data: unknown[]): Promise<void> {
    await set('songs', data)
  },

  // Setlists
  async loadSetlists() {
    return (await get('setlists')) || []
  },
  async saveSetlists(data: unknown[]): Promise<void> {
    await set('setlists', data)
  },

  // Settings
  async loadSettings(): Promise<Settings> {
    return (await get('settings')) || defaultSettings
  },
  async saveSettings(data: Settings): Promise<void> {
    await set('settings', data)
  },

  // Utility
  async clearAll(): Promise<void> {
    const allKeys = await keys()
    for (const key of allKeys) {
      await del(key)
    }
  },
}
