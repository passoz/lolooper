<script setup lang="ts">
import { ref, onMounted, onUnmounted, watch } from 'vue'
import type { IAState, IAAction } from '../worker/ia-worker'

const props = defineProps<{
  enabled: boolean
  mode: 'watch' | 'assist' | 'full'
  trackNames: string[]
  mutedTracks: string[]
  soloedTracks: string[]
  currentBar: number
  currentBeat: number
  currentStep: number
  currentBpm: number
  currentSection: string
  sectionsRemaining: number
}>()

const emit = defineEmits<{
  modeChange: [mode: 'watch' | 'assist' | 'full']
  toggle: [enabled: boolean]
  override: []
}>()

const actions = ref<IAAction[]>([])
const log = ref<string[]>([])
const energy = ref(0.5)
const worker = ref<Worker | null>(null)

function getEnergy(): number {
  // Simple energy heuristic based on section position and bpm
  const baseEnergy = props.currentBpm / 200  // 0.2 at 40bpm, 1.0 at 200bpm
  const sectionAdvance = props.sectionsRemaining > 0
    ? (props.sectionsRemaining / 8)  // more energy when fewer sections remain
    : 0.3
  return Math.min(1, Math.max(0, baseEnergy + sectionAdvance))
}

onMounted(() => {
  if (typeof Worker !== 'undefined') {
    worker.value = new Worker(
      new URL('../worker/ia-worker.ts', import.meta.url),
      { type: 'module' }
    )
    worker.value.onmessage = (e: MessageEvent<IAAction[]>) => {
      if (!props.enabled) return
      actions.value = e.data
      for (const action of e.data) {
        const confidence = Math.round((action.confidence || 0.5) * 100)
        log.value.unshift(
          `[${confidence}%] ${action.type}${action.track ? ': ' + action.track : ''}`
        )
        if (log.value.length > 50) log.value.pop()
      }
      if (props.mode === 'full') {
        // In full mode, automatically apply actions here
        // (the parent component handles the actual MIDI commands)
      }
    }
  }
})

onUnmounted(() => {
  worker.value?.terminate()
})

watch(() => [props.currentBar, props.currentBeat, props.currentStep, props.currentSection, props.enabled, props.mode] as const, () => {
  if (!props.enabled || !worker.value) return
  energy.value = getEnergy()
  const state: IAState = {
    bar: props.currentBar,
    beat: props.currentBeat,
    step: props.currentStep,
    section: props.currentSection,
    sectionsRemaining: props.sectionsRemaining,
    energy: energy.value,
    mode: props.mode,
    trackNames: props.trackNames,
    mutedTracks: props.mutedTracks,
    soloedTracks: props.soloedTracks,
    currentBpm: props.currentBpm,
  }
  worker.value.postMessage(state)
}, { immediate: true, deep: false })
</script>

<template>
  <div class="glass p-4 space-y-3">
    <!-- Header -->
    <div class="flex items-center justify-between">
      <h3 class="text-sm font-medium text-white/60 uppercase tracking-wider">IA</h3>
      <label class="flex items-center gap-2 cursor-pointer">
        <span class="text-xs text-white/40">{{ enabled ? 'Ligada' : 'Desligada' }}</span>
        <div class="w-9 h-5 rounded-full transition-colors duration-200 relative"
          :class="enabled ? 'bg-indigo-500/60' : 'bg-white/10'"
          @click="emit('toggle', !enabled)">
          <div class="w-3.5 h-3.5 rounded-full bg-white absolute top-0.5 transition-all duration-200"
            :class="enabled ? 'left-[18px]' : 'left-[3px]'" />
        </div>
      </label>
    </div>

    <!-- Mode selector -->
    <div v-if="enabled" class="flex gap-1">
      <button v-for="m in ['watch', 'assist', 'full'] as const" :key="m"
        class="flex-1 px-2 py-1 text-xs rounded-lg transition-all cursor-pointer"
        :class="mode === m
          ? 'bg-indigo-500/30 text-indigo-300 border border-indigo-500/30'
          : 'bg-white/10 text-white/50 hover:bg-white/20 border border-white/10'"
        @click="emit('modeChange', m)">
        {{ { watch: 'Observar', assist: 'Auxiliar', full: 'Tomar Conta' }[m] }}
      </button>
    </div>

    <!-- Energy indicator -->
    <div v-if="enabled">
      <div class="flex justify-between text-xs text-white/40 mb-1">
        <span>Energia</span>
        <span>{{ Math.round(energy * 100) }}%</span>
      </div>
      <div class="h-2 bg-white/10 rounded-full overflow-hidden">
        <div class="h-full rounded-full transition-all duration-500"
          :style="{
            width: (energy * 100) + '%',
            background: energy > 0.7 ? '#22c55e' : energy > 0.4 ? '#eab308' : '#6366f1'
          }" />
      </div>
    </div>

    <!-- Action log -->
    <div v-if="enabled && log.length > 0" class="max-h-32 overflow-y-auto space-y-0.5">
      <div v-for="(entry, i) in log.slice(0, 10)" :key="i"
        class="text-[10px] font-mono text-white/40 leading-tight">
        {{ entry }}
      </div>
    </div>

    <!-- Override button -->
    <button v-if="enabled && mode === 'full'" class="w-full px-2 py-1 text-xs text-yellow-400/60
      hover:text-yellow-400 bg-yellow-500/10 hover:bg-yellow-500/20 rounded-lg transition-colors cursor-pointer"
      @click="emit('override')">
      Override: ignorar IA
    </button>
  </div>
</template>
