<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue'
import PatternGrid from '../components/PatternGrid.vue'
import TrackList from '../components/TrackList.vue'
import Transport from '../components/Transport.vue'
import IAPanel from '../components/IAPanel.vue'
import { usePatternsStore } from '../stores/patterns'
import { useMidiStore } from '../stores/midi'
import type { TransportState } from '../stores/midi'

const patternsStore = usePatternsStore()
const midiStore = useMidiStore()

const trackNames = [
  'Surdo 1', 'Surdo 2', 'Surdo 3', 'Caixa', 'Repique',
  'Tamborim', 'Pandeiro', 'Cuica', 'Agogo', 'Reco-reco',
  'Tantan', 'Cavaquinho', 'Violao 7', 'Banjo',
]

const isPlaying = ref(false)
const isRecording = ref(false)
const bpm = ref(100)
const swing = ref(0)
const humanize = ref(0)
const activeStep = ref(-1)
const currentBar = ref(0)
const currentBeat = ref(0)
const currentStep = ref(0)

// IA state
const iaEnabled = ref(false)
const iaMode = ref<'watch' | 'assist' | 'full'>('watch')
const mutedTrackNames = ref<string[]>([])
const soloedTrackNames = ref<string[]>([])

// Song progress (from SysEx 0x06)
const songSectionIndex = ref(0)
const songSectionBars = ref(0)
const songTotalSections = ref(0)
const songModeActive = ref(false)

let stepInterval: ReturnType<typeof setInterval> | null = null

function onTransportUpdate(state: TransportState) {
  // Sync from plugin SysEx 0x05
  isPlaying.value = state.playing
  isRecording.value = state.recording
  currentBar.value = state.bar
  currentBeat.value = state.beat
  currentStep.value = state.step
  activeStep.value = state.step

  // If playing, don't use local interval anymore
  if (state.playing) {
    if (stepInterval) {
      clearInterval(stepInterval)
      stepInterval = null
    }
  }
}

onMounted(async () => {
  await patternsStore.loadFromDB()
  if (patternsStore.styles.size === 0) {
    patternsStore.loadDefaults()
  }

  // Keyboard shortcuts
  const onKeyDown = (e: KeyboardEvent) => {
    if (e.ctrlKey || e.metaKey) {
      const key = e.key.toLowerCase()
      if (key === 'z' && !e.shiftKey) { patternsStore.undo(); e.preventDefault() }
      if (key === 'z' && e.shiftKey) { patternsStore.redo(); e.preventDefault() }
      if (key === 'y') { patternsStore.redo(); e.preventDefault() }
    }
  }
  document.addEventListener('keydown', onKeyDown)
  onUnmounted(() => document.removeEventListener('keydown', onKeyDown))

  // Listen for SysEx transport state from plugin
  midiStore.onTransportState = onTransportUpdate

  // Listen for SysEx 0x06 song feedback
  midiStore.onSongFeedback = (data) => {
    songSectionIndex.value = data.section
    songSectionBars.value = data.barsDone
    songTotalSections.value = data.totalSections
    songModeActive.value = data.totalSections > 0
  }
})

function togglePlay() {
  if (isPlaying.value) {
    isPlaying.value = false
    if (stepInterval) clearInterval(stepInterval)
    activeStep.value = -1
  } else {
    isPlaying.value = true
    activeStep.value = 0
    // Simulate step advancement (visual only)
    stepInterval = setInterval(() => {
      const bpmVal = bpm.value
      const stepDuration = (60 / bpmVal / 4) * 1000 // 16th note in ms
      activeStep.value = (activeStep.value + 1) % 16
      if (activeStep.value === 0) {
        currentBeat.value = (currentBeat.value + 1) % 4
        if (currentBeat.value === 0) currentBar.value++
      }
    }, (60 / bpm.value / 4) * 1000)
  }
}

function toggleRecord() {
  isRecording.value = !isRecording.value
  if (isRecording.value) {
    midiStore.sendNote(117) // REC on
  } else {
    midiStore.sendNote(117) // REC off
  }
}

function handleStop() {
  isPlaying.value = false
  isRecording.value = false
  if (stepInterval) clearInterval(stepInterval)
  activeStep.value = -1
  currentBar.value = 0
  currentBeat.value = 0
  currentStep.value = 0
  midiStore.sendNote(115) // Stop
}

function handleBpmChange(newBpm: number) {
  bpm.value = newBpm
  midiStore.sendCC(15, Math.round((newBpm - 40) / 160 * 127))
  // Restart step interval with new BPM
  if (isPlaying.value) {
    if (stepInterval) clearInterval(stepInterval)
    stepInterval = setInterval(() => {
      activeStep.value = (activeStep.value + 1) % 16
    }, (60 / bpm.value / 4) * 1000)
  }
}

function handleSwingChange(newSwing: number) {
  swing.value = newSwing
  midiStore.sendCC(16, Math.round(newSwing / 100 * 127))
}

function handleCycle(track: number, step: number) {
  patternsStore.cycleVelocity(track, step)
  sendPatternToPlugin()
}

// Copy/paste/clear handlers
function handleCopyTrack(track: number) {
  patternsStore.copyTrack(track)
}

function handlePasteTrack(track: number) {
  patternsStore.pasteTrack(track)
  sendPatternToPlugin()
}

function handleClearTrack(track: number) {
  patternsStore.clearTrack(track)
  sendPatternToPlugin()
}

function sendPatternToPlugin() {
  const pattern = patternsStore.activePattern
  if (pattern) {
    const data = JSON.stringify({
      style: patternsStore.activeStyle,
      grid: pattern.grid,
    })
    midiStore.sendSysEx([0x03, ...new TextEncoder().encode(data)])
  }
}

// Style selector
const availableStyles = ref(['samba', 'pagode', 'partido_alto', 'samba_reggae', 'ijexa', 'frevo', 'maracatu', 'intro', 'virada'])
const showStyleMenu = ref(false)

function selectStyle(style: string) {
  patternsStore.activeStyle = style
  showStyleMenu.value = false
  midiStore.sendCC(18, Math.max(0, availableStyles.value.indexOf(style)))
}
</script>

<template>
  <div class="space-y-4">
    <!-- Header with style selector -->
    <div class="flex items-center justify-between">
      <div class="flex items-center gap-3">
        <h2 class="text-lg font-bold">Pattern Editor</h2>
        <div class="relative">
          <button
            class="px-3 py-1.5 glass text-sm flex items-center gap-2 cursor-pointer"
            @click="showStyleMenu = !showStyleMenu"
          >
            {{ patternsStore.activeStyle }}
            <span class="text-xs text-white/40">▼</span>
          </button>
          <div
            v-if="showStyleMenu"
            class="absolute top-full left-0 mt-1 glass p-1 z-10 min-w-32"
            @click.self="showStyleMenu = false"
          >
            <button
              v-for="style in availableStyles"
              :key="style"
              class="block w-full text-left px-3 py-1.5 text-sm rounded-lg hover:bg-white/10 transition-colors"
              :class="style === patternsStore.activeStyle ? 'bg-indigo-500/20 text-indigo-300' : 'text-white/70'"
              @click="selectStyle(style)"
            >
              {{ style }}
            </button>
          </div>
        </div>
      </div>

      <div class="flex gap-2">
        <button
          class="px-3 py-1.5 text-xs glass text-white/60 hover:text-white/80 cursor-pointer"
          @click="patternsStore.addStyle(`Estilo ${patternsStore.styles.size + 1}`)"
        >
          + Novo Estilo
        </button>
        <button
          class="px-3 py-1.5 text-xs glass text-white/60 hover:text-white/80 cursor-pointer"
          @click="() => { const blob = new Blob([patternsStore.exportJson()], { type: 'application/json' }); const url = URL.createObjectURL(blob); const a = document.createElement('a'); a.href = url; a.download = 'patterns.json'; a.click() }"
        >
          Exportar
        </button>
        <label class="px-3 py-1.5 text-xs glass text-white/60 hover:text-white/80 cursor-pointer inline-flex items-center gap-1">
          Importar
          <input type="file" accept=".json" class="hidden"
            @change="(e) => { const file = (e.target as HTMLInputElement).files?.[0]; if (!file) return; file.text().then(patternsStore.importJson).catch(err => console.error('Import failed:', err)); }" />
        </label>
      </div>
    </div>

    <!-- Song progress (from plugin feedback) -->
    <div v-if="songModeActive" class="glass px-4 py-2 flex items-center gap-3">
      <span class="text-xs text-white/40 uppercase tracking-wider">Song</span>
      <div class="flex items-center gap-1 text-sm">
        <span class="text-indigo-400">Seção {{ songSectionIndex + 1 }}/{{ songTotalSections }}</span>
        <span class="text-white/30">·</span>
        <span class="text-white/60">Compasso {{ songSectionBars + 1 }}</span>
      </div>
      <div class="flex-1 h-1.5 bg-white/10 rounded-full overflow-hidden">
        <div class="h-full bg-indigo-500/60 rounded-full transition-all duration-300"
          :style="{ width: songTotalSections > 0 ? ((songSectionIndex + 1) / songTotalSections * 100) + '%' : '0%' }" />
      </div>
    </div>

    <!-- Transport -->
    <Transport
      :isPlaying="isPlaying"
      :isRecording="isRecording"
      :bpm="bpm"
      :swing="swing"
      :humanize="humanize"
      :currentBar="currentBar"
      :currentBeat="currentBeat"
      :currentStep="currentStep"
      @play="togglePlay"
      @stop="handleStop"
      @record="toggleRecord"
      @bpmChange="handleBpmChange"
      @swingChange="handleSwingChange"
      @humanizeChange="(v) => { humanize = v; midiStore.sendCC(17, Math.round(v / 50 * 127)) }"
    />

    <!-- Main layout -->
    <div class="grid grid-cols-1 xl:grid-cols-[1fr_300px] gap-4">
      <!-- Pattern Grid -->
      <PatternGrid
        v-if="patternsStore.activePattern"
        :grid="patternsStore.activePattern.grid"
        :activeStep="activeStep"
        :trackNames="trackNames"
        :clipTrackGrid="patternsStore.clipTrackGrid"
        @cycle="handleCycle"
        @copyTrack="handleCopyTrack"
        @pasteTrack="handlePasteTrack"
        @clearTrack="handleClearTrack"
      />

      <!-- Track List -->
      <TrackList :trackNames="trackNames" />

      <!-- IA Panel -->
      <IAPanel
        :enabled="iaEnabled"
        :mode="iaMode"
        :trackNames="trackNames"
        :mutedTracks="mutedTrackNames"
        :soloedTracks="soloedTrackNames"
        :currentBar="currentBar"
        :currentBeat="currentBeat"
        :currentStep="currentStep"
        :currentBpm="bpm"
        :currentSection="patternsStore.activeStyle"
        :sectionsRemaining="songTotalSections - songSectionIndex - 1"
        @toggle="(v) => iaEnabled = v"
        @modeChange="(m) => iaMode = m"
        @override="iaMode = 'assist'"
      />
    </div>
  </div>
</template>
