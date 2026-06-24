<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useMidiStore } from '../stores/midi'
import { db } from '../db/database'

const midiStore = useMidiStore()

interface MidiPort {
  id: string
  name: string
}

const midiOutputs = ref<MidiPort[]>([])
const selectedOutput = ref('')
const iaMode = ref<'off' | 'assist' | 'watch' | 'full'>('off')

onMounted(async () => {
  if (midiStore.connected) {
    midiOutputs.value = midiStore.outputs.map(o => ({
      id: o.id,
      name: o.name || 'Unknown',
    }))
  }
  const settings = await db.loadSettings()
  iaMode.value = settings.iaMode || 'off'
  selectedOutput.value = settings.midiOutputId || ''
})

function selectOutput(id: string) {
  selectedOutput.value = id
  db.saveSettings({
    midiOutputId: id,
    iaMode: iaMode.value,
    theme: 'dark',
  })
}

function setIaMode(mode: typeof iaMode.value) {
  iaMode.value = mode
  db.saveSettings({
    midiOutputId: selectedOutput.value,
    iaMode: mode,
    theme: 'dark',
  })
}
</script>

<template>
  <div class="space-y-6">
    <h2 class="text-lg font-bold">Configurações</h2>

    <!-- MIDI Settings -->
    <div class="glass p-4 space-y-3">
      <h3 class="text-sm font-medium text-white/60 uppercase tracking-wider">MIDI</h3>

      <div class="flex items-center gap-2">
        <span
          class="w-2 h-2 rounded-full"
          :class="midiStore.connected ? 'bg-green-500' : 'bg-red-500'"
        />
        <span class="text-sm text-white/70">
          {{ midiStore.connected ? 'Conectado' : 'Desconectado' }}
        </span>
      </div>

      <div class="space-y-1">
        <label class="text-xs text-white/40">Saída MIDI</label>
        <select
          v-if="midiOutputs.length > 0"
          :value="selectedOutput"
          class="w-full px-3 py-2 bg-white/10 border border-white/20 rounded-lg text-white text-sm outline-none focus:border-indigo-500/50"
          @change="selectOutput(($event.target as HTMLSelectElement).value)"
        >
          <option value="" disabled>Selecione uma porta...</option>
          <option
            v-for="port in midiOutputs"
            :key="port.id"
            :value="port.id"
          >
            {{ port.name }}
          </option>
        </select>
        <p v-else class="text-sm text-white/40">Nenhuma porta MIDI encontrada</p>
      </div>
    </div>

    <!-- IA Settings -->
    <div class="glass p-4 space-y-3">
      <h3 class="text-sm font-medium text-white/60 uppercase tracking-wider">IA</h3>
      <div class="flex gap-2">
        <button
          v-for="mode in ['off', 'watch', 'assist', 'full'] as const"
          :key="mode"
          class="px-4 py-2 text-sm rounded-lg transition-all cursor-pointer"
          :class="iaMode === mode
            ? 'bg-indigo-500/30 text-indigo-300 border border-indigo-500/30'
            : 'bg-white/10 text-white/50 hover:bg-white/20 border border-white/10'"
          @click="setIaMode(mode)"
        >
          {{ { off: 'Desligada', watch: 'Observar', assist: 'Auxiliar', full: 'Tomar Conta' }[mode] }}
        </button>
      </div>
    </div>

    <!-- About -->
    <div class="glass p-4 space-y-2">
      <h3 class="text-sm font-medium text-white/60 uppercase tracking-wider">Sobre</h3>
      <p class="text-sm text-white/40">Lolooper v1.0.0</p>
      <p class="text-sm text-white/40">Live performance looper for Brazilian music</p>
    </div>
  </div>
</template>
