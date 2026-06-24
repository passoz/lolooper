import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { db } from '../db/database'
import { defaultPatterns } from '../data/defaultPatterns'

const VELOCITIES = [0, 0.4, 0.7, 1.0] as const
const MAX_HISTORY = 50

function cloneGrid(grid: number[][]): number[][] {
  return grid.map(track => [...track])
}

export interface Pattern {
  name: string
  grid: number[][]  // 14 tracks × 16 steps
}

export const usePatternsStore = defineStore('patterns', () => {
  const styles = ref<Map<string, Pattern>>(new Map())
  const activeStyle = ref('samba')
  const history = ref<number[][][]>([])  // snapshots of active grid
  const historyIndex = ref(-1)

  const canUndo = computed(() => historyIndex.value > 0)
  const canRedo = computed(() => historyIndex.value < history.value.length - 1)

  const activePattern = computed(() => styles.value.get(activeStyle.value))

  function createEmptyGrid(): number[][] {
    return Array.from({ length: 14 }, () => Array(16).fill(0))
  }

  /** Push grid state AFTER mutation to undo stack. */
  function recordSnapshot() {
    const pattern = styles.value.get(activeStyle.value)
    if (!pattern) return
    // Discard any redo states beyond current index
    history.value = history.value.slice(0, historyIndex.value + 1)
    history.value.push(cloneGrid(pattern.grid))
    // Cap size
    if (history.value.length > MAX_HISTORY) history.value.shift()
    historyIndex.value = history.value.length - 1
  }

  function cycleVelocity(track: number, step: number) {
    const pattern = styles.value.get(activeStyle.value)
    if (!pattern) return
    const current = pattern.grid[track][step]
    const currentIdx = VELOCITIES.indexOf(current as typeof VELOCITIES[number])
    const nextIdx = (currentIdx + 1) % VELOCITIES.length
    pattern.grid[track][step] = VELOCITIES[nextIdx]
    recordSnapshot()
    saveToDB()
  }

  function setVelocity(track: number, step: number, vel: number) {
    const pattern = styles.value.get(activeStyle.value)
    if (!pattern) return
    pattern.grid[track][step] = vel
    recordSnapshot()
    saveToDB()
  }

  function setPattern(name: string, grid: number[][]) {
    styles.value.set(name, { name, grid: cloneGrid(grid) })
    if (name === activeStyle.value) recordSnapshot()
    saveToDB()
  }

  function setRange(trackStart: number, trackEnd: number,
                    stepStart: number, stepEnd: number, vel: number) {
    const pattern = styles.value.get(activeStyle.value)
    if (!pattern) return
    for (let t = trackStart; t <= trackEnd; t++) {
      for (let s = stepStart; s <= stepEnd; s++) {
        if (t >= 0 && t < 14 && s >= 0 && s < 16)
          pattern.grid[t][s] = vel
      }
    }
    recordSnapshot()
    saveToDB()
  }

  function undo() {
    if (!canUndo.value) return
    historyIndex.value--
    const pattern = styles.value.get(activeStyle.value)
    if (pattern) pattern.grid = cloneGrid(history.value[historyIndex.value])
    saveToDB()
  }

  function redo() {
    if (!canRedo.value) return
    historyIndex.value++
    const pattern = styles.value.get(activeStyle.value)
    if (pattern) pattern.grid = cloneGrid(history.value[historyIndex.value])
    saveToDB()
  }

  function addStyle(name: string) {
    if (styles.value.has(name)) return
    const grid = createEmptyGrid()
    styles.value.set(name, { name, grid })
    activeStyle.value = name
    // Reset undo when switching to a new empty style
    history.value = [cloneGrid(grid)]
    historyIndex.value = 0
    saveToDB()
  }

  function exportJson(): string {
    const obj: Record<string, number[][]> = {}
    styles.value.forEach((p, name) => { obj[name] = p.grid })
    return JSON.stringify(obj, null, 2)
  }

  function importJson(json: string) {
    const obj = JSON.parse(json) as Record<string, number[][]>
    Object.entries(obj).forEach(([name, grid]) => {
      styles.value.set(name, { name, grid })
    })
    saveToDB()
  }

  async function loadFromDB() {
    const saved = await db.loadPatterns()
    if (saved && Object.keys(saved).length > 0) {
      Object.entries(saved).forEach(([name, grid]) => {
        styles.value.set(name, { name, grid: grid as number[][] })
      })
    }
  }

  function saveToDB() {
    const obj: Record<string, number[][]> = {}
    styles.value.forEach((p) => { obj[p.name] = p.grid })
    db.savePatterns(obj)
  }

  function loadDefaults() {
    Object.entries(defaultPatterns).forEach(([name, grid]) => {
      if (!styles.value.has(name)) {
        styles.value.set(name, { name, grid: cloneGrid(grid) })
      }
    })
    if (!activeStyle.value || !styles.value.has(activeStyle.value)) {
      activeStyle.value = 'samba'
    }
    saveToDB()
  }

  // Clipboard for copy/paste track
  const clipTrackGrid = ref<number[] | null>(null)

  function copyTrack(track: number) {
    const pattern = styles.value.get(activeStyle.value)
    if (!pattern) return
    clipTrackGrid.value = [...pattern.grid[track]]
  }

  function pasteTrack(track: number) {
    if (!clipTrackGrid.value) return
    const pattern = styles.value.get(activeStyle.value)
    if (!pattern) return
    pattern.grid[track] = [...clipTrackGrid.value]
    recordSnapshot()
    saveToDB()
  }

  function clearTrack(track: number) {
    const pattern = styles.value.get(activeStyle.value)
    if (!pattern) return
    pattern.grid[track] = Array(16).fill(0)
    recordSnapshot()
    saveToDB()
  }

  return {
    styles, activeStyle, activePattern,
    canUndo, canRedo, history, historyIndex,
    clipTrackGrid,
    cycleVelocity, setVelocity, setPattern, setRange,
    copyTrack, pasteTrack, clearTrack,
    undo, redo, addStyle,
    exportJson, importJson, loadFromDB, loadDefaults,
  }
})
