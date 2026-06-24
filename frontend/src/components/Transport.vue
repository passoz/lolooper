<script setup lang="ts">
import { ref } from 'vue'

const emit = defineEmits<{
  play: []
  stop: []
  record: []
  bpmChange: [bpm: number]
  swingChange: [swing: number]
  humanizeChange: [humanize: number]
}>()

defineProps<{
  isPlaying: boolean
  isRecording: boolean
  bpm: number
  swing: number
  humanize: number
  currentBar?: number
  currentBeat?: number
  currentStep?: number
}>()

const editingBpm = ref(false)
const tempBpm = ref(100)

function startEditBpm(currentBpm: number) {
  tempBpm.value = currentBpm
  editingBpm.value = true
}

function commitBpm() {
  emit('bpmChange', tempBpm.value)
  editingBpm.value = false
}
</script>

<template>
  <div class="glass p-4">
    <div class="flex items-center gap-4 flex-wrap">
      <!-- Transport buttons -->
      <button
        class="w-12 h-12 rounded-full flex items-center justify-center text-lg transition-all duration-200"
        :class="isPlaying
          ? 'bg-indigo-500/30 text-indigo-300 border border-indigo-500/40 shadow-[0_0_12px_rgba(99,102,241,0.3)]'
          : 'bg-white/10 text-white/70 hover:bg-white/20 border border-white/10'"
        @click="emit(isPlaying ? 'stop' : 'play')"
      >
        {{ isPlaying ? '⏸' : '▶' }}
      </button>

      <button
        class="w-12 h-12 rounded-full flex items-center justify-center text-lg transition-all duration-200"
        :class="isRecording
          ? 'bg-red-500/30 text-red-300 border border-red-500/40 shadow-[0_0_12px_rgba(239,68,68,0.3)]'
          : 'bg-white/10 text-white/70 hover:bg-white/20 border border-white/10'"
        @click="emit('record')"
      >
        ⏺
      </button>

      <button
        class="w-12 h-12 rounded-full flex items-center justify-center bg-white/10 text-white/70 hover:bg-white/20 border border-white/10 transition-all duration-200"
        @click="emit('stop')"
      >
        ⏹
      </button>

      <!-- Divider -->
      <div class="w-px h-8 bg-white/10" />

      <!-- BPM -->
      <div class="flex items-center gap-2">
        <span class="text-xs text-white/40">BPM</span>
        <div v-if="editingBpm" class="flex items-center gap-1">
          <input
            v-model.number="tempBpm"
            type="number"
            min="20"
            max="300"
            class="w-16 px-2 py-1 bg-white/10 border border-white/20 rounded-lg text-center text-sm font-mono text-white"
            @keyup.enter="commitBpm"
            @blur="commitBpm"
          />
        </div>
        <div
          v-else
          class="px-3 py-1 bg-white/10 rounded-lg cursor-pointer text-sm font-mono hover:bg-white/20 transition-colors"
          @click="startEditBpm(bpm)"
        >
          {{ bpm }}
        </div>
      </div>

      <!-- Swing -->
      <div class="flex items-center gap-2">
        <span class="text-xs text-white/40">Swing</span>
        <input
          type="range"
          min="0"
          max="100"
          :value="swing"
          class="w-20 accent-indigo-500"
          @input="emit('swingChange', Number(($event.target as HTMLInputElement).value))"
        />
        <span class="text-xs font-mono text-white/60 w-8">{{ swing }}%</span>
      </div>

      <!-- Humanize -->
      <div class="flex items-center gap-2">
        <span class="text-xs text-white/40">Human</span>
        <input
          type="range"
          min="0"
          max="50"
          :value="humanize"
          class="w-20 accent-indigo-500"
          @input="emit('humanizeChange', Number(($event.target as HTMLInputElement).value))"
        />
        <span class="text-xs font-mono text-white/60 w-8">{{ humanize }}ms</span>
      </div>

      <!-- Position indicator -->
      <div class="flex items-center gap-2 ml-auto text-sm font-mono text-white/60">
        <span class="text-indigo-400 font-bold">
          {{ currentBar || 0 }}.{{ (currentBeat || 0) + 1 }}.{{ (currentStep || 0) + 1 }}
        </span>
      </div>
    </div>
  </div>
</template>
