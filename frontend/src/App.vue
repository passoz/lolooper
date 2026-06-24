<script setup lang="ts">
import { useMidiStore } from './stores/midi'
import { ref, onMounted } from 'vue'
import { useRouter, useRoute } from 'vue-router'

const midiStore = useMidiStore()
const router = useRouter()
const route = useRoute()

const navItems = [
  { path: '/editor', label: 'Editor', icon: '🎛' },
  { path: '/songs', label: 'Músicas', icon: '🎵' },
  { path: '/setlists', label: 'Setlists', icon: '📋' },
  { path: '/settings', label: 'Config', icon: '⚙' },
]

onMounted(async () => {
  try {
    await midiStore.connect()
  } catch {
    console.warn('MIDI not available')
  }
})
</script>

<template>
  <div class="min-h-screen bg-[#0a0a0a] text-white">
    <!-- Header -->
    <header class="glass mx-4 mt-4 px-6 py-3 flex items-center justify-between">
      <div class="flex items-center gap-3">
        <span class="text-2xl">🥁</span>
        <h1 class="text-xl font-bold tracking-tight">Lolooper</h1>
      </div>

      <!-- Navigation -->
      <nav class="flex gap-2">
        <router-link
          v-for="item in navItems"
          :key="item.path"
          :to="item.path"
          class="px-4 py-2 rounded-xl text-sm transition-all duration-200"
          :class="route.path === item.path
            ? 'bg-white/15 text-white border border-white/20'
            : 'text-white/50 hover:text-white/80 hover:bg-white/5'"
        >
          {{ item.icon }} {{ item.label }}
        </router-link>
      </nav>

      <!-- MIDI Status -->
      <div class="flex items-center gap-2 text-sm">
        <span
          class="w-2 h-2 rounded-full"
          :class="midiStore.connected ? 'bg-green-500 shadow-[0_0_8px_rgba(34,197,94,0.5)]' : 'bg-red-500'"
        />
        <span class="text-white/50">MIDI</span>
      </div>
    </header>

    <!-- Main Content -->
    <main class="mx-4 mt-4 p-6">
      <router-view />
    </main>
  </div>
</template>
