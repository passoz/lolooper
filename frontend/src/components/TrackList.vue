<script setup lang="ts">
import { ref } from 'vue'
import { useMidiStore } from '../stores/midi'

const props = defineProps<{
  trackNames: string[]
}>()

const midiStore = useMidiStore()
const muteStates = ref(new Array(14).fill(false))
const soloStates = ref(new Array(14).fill(false))
const volumes = ref(new Array(14).fill(0.8))
const pans = ref(new Array(14).fill(0.5))

function toggleMute(track: number) {
  muteStates.value[track] = !muteStates.value[track]
  // Send MIDI CC: note 36-49
  midiStore.sendNote(36 + track, muteStates.value[track] ? 127 : 0)
}

function toggleSolo(track: number) {
  soloStates.value[track] = !soloStates.value[track]
}

function updateVolume(track: number, value: number) {
  volumes.value[track] = value
  // Send MIDI CC 20-33
  midiStore.sendCC(20 + track, Math.round(value * 127))
}

function updatePan(track: number, value: number) {
  pans.value[track] = value
  // Send MIDI CC 34-47
  midiStore.sendCC(34 + track, Math.round(value * 127))
}
</script>

<template>
  <div class="glass p-4">
    <h3 class="text-xs font-medium text-white/40 uppercase tracking-wider mb-3">Tracks</h3>
    <div class="space-y-1">
      <div
        v-for="(name, tIdx) in trackNames"
        :key="tIdx"
        class="flex items-center gap-2 py-1.5 px-2 rounded-lg transition-colors"
        :class="muteStates[tIdx] ? 'bg-white/5 opacity-50' : 'hover:bg-white/5'"
      >
        <!-- Track name -->
        <span class="w-24 text-sm text-white/80 truncate">{{ name }}</span>

        <!-- Mute -->
        <button
          class="w-7 h-7 rounded text-xs font-bold transition-all duration-150"
          :class="muteStates[tIdx]
            ? 'bg-red-500/40 text-red-300 border border-red-500/30'
            : 'bg-white/10 text-white/50 hover:bg-white/20 border border-white/10'"
          @click="toggleMute(tIdx)"
        >
          M
        </button>

        <!-- Solo -->
        <button
          class="w-7 h-7 rounded text-xs font-bold transition-all duration-150"
          :class="soloStates[tIdx]
            ? 'bg-yellow-500/40 text-yellow-300 border border-yellow-500/30'
            : 'bg-white/10 text-white/50 hover:bg-white/20 border border-white/10'"
          @click="toggleSolo(tIdx)"
        >
          S
        </button>

        <!-- Volume slider -->
        <div class="flex-1 flex items-center gap-1">
          <span class="text-[10px] text-white/30 w-4 text-right">{{ Math.round(volumes[tIdx] * 100) }}</span>
          <input
            type="range"
            min="0"
            max="1"
            step="0.01"
            :value="volumes[tIdx]"
            class="flex-1 accent-indigo-500 h-1"
            @input="updateVolume(tIdx, Number(($event.target as HTMLInputElement).value))"
          />
        </div>

        <!-- Pan slider -->
        <div class="w-16 flex items-center gap-1">
          <span class="text-[10px] text-white/30 w-6 text-right">{{ pans[tIdx].toFixed(1) }}</span>
          <input
            type="range"
            min="0"
            max="1"
            step="0.05"
            :value="pans[tIdx]"
            class="flex-1 accent-indigo-500 h-1"
            @input="updatePan(tIdx, Number(($event.target as HTMLInputElement).value))"
          />
        </div>
      </div>
    </div>
  </div>
</template>
