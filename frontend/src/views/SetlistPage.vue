<script setup lang="ts">
import { ref, computed } from 'vue'
import { useSetlistStore } from '../stores/setlist'
import { useSongsStore } from '../stores/songs'
import { useMidiStore } from '../stores/midi'

const setlistStore = useSetlistStore()
const songsStore = useSongsStore()
const midiStore = useMidiStore()

const showNewSetlist = ref(false)
const newSetlistName = ref('')
const editingSetlistId = ref<string | null>(null)
const showAddSong = ref(false)

function createSetlist() {
  if (!newSetlistName.value.trim()) return
  setlistStore.createSetlist(newSetlistName.value.trim())
  newSetlistName.value = ''
  showNewSetlist.value = false
}

const editingSetlist = computed(() => {
  if (!editingSetlistId.value) return null
  return setlistStore.setlists.find(s => s.id === editingSetlistId.value) || null
})

function openSetlist(id: string) {
  editingSetlistId.value = id
  showAddSong.value = false
}

function closeSetlist() {
  editingSetlistId.value = null
}

// Songs not already in the setlist
const availableSongs = computed(() => {
  if (!editingSetlist.value) return []
  const idsInSetlist = new Set(editingSetlist.value.songs)
  return songsStore.songs.filter(s => !idsInSetlist.has(s.id))
})

function toggleAddSong() {
  showAddSong.value = !showAddSong.value
}

function addSongToSetlist(songId: string) {
  if (!editingSetlist.value) return
  setlistStore.addSongToSetlist(editingSetlist.value.id, songId)
  showAddSong.value = false
}

function removeSongFromSetlist(songId: string) {
  if (!editingSetlist.value) return
  setlistStore.removeSongFromSetlist(editingSetlist.value.id, songId)
}

function moveSong(from: number, to: number) {
  if (!editingSetlist.value) return
  setlistStore.reorder(editingSetlist.value.id, from, to)
}

function getSongById(id: string) {
  return songsStore.songs.find(s => s.id === id)
}

// Send button — send whole setlist info via SysEx
function sendSetlistToPlugin() {
  if (!editingSetlist.value) return
  const data = JSON.stringify({
    type: 'setlist',
    name: editingSetlist.value.name,
    songIds: editingSetlist.value.songs,
  })
  midiStore.sendSysEx([0x04, ...new TextEncoder().encode(data)])
}
</script>

<template>
  <div class="space-y-4">
    <!-- Header + Create -->
    <template v-if="!editingSetlistId">
      <div class="flex items-center justify-between">
        <h2 class="text-lg font-bold">Setlists</h2>
        <button class="px-4 py-2 glass text-sm cursor-pointer hover:bg-white/10 transition-colors"
                @click="showNewSetlist = !showNewSetlist">
          + Nova Setlist
        </button>
      </div>

      <div v-if="showNewSetlist" class="glass p-4 space-y-3">
        <input v-model="newSetlistName" type="text"
          placeholder="Nome da setlist (ex: Show 15/07)"
          class="w-full px-3 py-2 bg-white/10 border border-white/20 rounded-lg text-white placeholder-white/30 outline-none"
          @keyup.enter="createSetlist" />
        <button class="px-4 py-1.5 bg-indigo-500/30 text-indigo-300 rounded-lg text-sm
                       hover:bg-indigo-500/50 transition-colors cursor-pointer"
                @click="createSetlist">Criar</button>
      </div>

      <div v-if="setlistStore.setlists.length === 0" class="glass p-8 text-center text-white/40">
        <p class="text-4xl mb-4">📋</p>
        <p>Nenhuma setlist ainda. Crie a primeira!</p>
      </div>

      <div v-else class="space-y-2">
        <div v-for="setlist in setlistStore.setlists" :key="setlist.id"
          class="glass p-4 hover:bg-white/[0.07] transition-colors cursor-pointer flex items-center justify-between"
          @click="openSetlist(setlist.id)">
          <div>
            <h3 class="font-medium">{{ setlist.name }}</h3>
            <p class="text-sm text-white/40">{{ setlist.songs.length }} músicas · {{ setlist.date }}</p>
          </div>
          <span class="text-xs text-white/30 hover:text-white/60">→</span>
        </div>
      </div>
    </template>

    <!-- Setlist Detail -->
    <template v-else-if="editingSetlist">
      <div class="glass p-4 space-y-4">
        <div class="flex items-center justify-between">
          <div>
            <h2 class="text-lg font-bold">{{ editingSetlist.name }}</h2>
            <p class="text-sm text-white/40">{{ editingSetlist.songs.length }} músicas · {{ editingSetlist.date }}</p>
          </div>
          <div class="flex gap-2">
            <button class="px-3 py-1.5 text-xs glass text-indigo-300 hover:bg-indigo-500/20
                           transition-colors cursor-pointer"
                    @click="sendSetlistToPlugin">Enviar</button>
            <button class="px-3 py-1.5 text-xs glass text-white/60 hover:text-white/80
                           transition-colors cursor-pointer"
                    @click="closeSetlist">Voltar</button>
          </div>
        </div>

        <!-- Songs in setlist -->
        <div>
          <div class="flex items-center justify-between mb-2">
            <h3 class="text-sm font-medium text-white/60 uppercase tracking-wider">Músicas</h3>
            <button class="px-3 py-1 text-xs glass text-indigo-300 hover:bg-indigo-500/20
                           transition-colors cursor-pointer"
                    @click="toggleAddSong">+ Adicionar</button>
          </div>

          <!-- Add song selector -->
          <div v-if="showAddSong" class="glass p-2 mb-2">
            <div v-if="availableSongs.length === 0" class="text-xs text-white/40 p-2">
              Todas as músicas já estão na setlist.
            </div>
            <button v-for="song in availableSongs" :key="song.id"
              class="block w-full text-left px-3 py-2 text-sm rounded-lg hover:bg-white/10
                     transition-colors text-white/70 cursor-pointer"
              @click="addSongToSetlist(song.id)">
              {{ song.name }} ({{ song.bpm }} BPM)
            </button>
          </div>

          <div v-if="editingSetlist.songs.length === 0" class="text-sm text-white/40 py-4 text-center">
            Nenhuma música na setlist. Adicione músicas para o show.
          </div>

          <div v-else class="space-y-1">
            <div v-for="(songId, sIdx) in editingSetlist.songs" :key="songId"
              class="flex items-center gap-2 py-2 px-3 rounded-lg hover:bg-white/5 transition-colors group">
              <span class="text-xs text-white/30 w-6">{{ sIdx + 1 }}.</span>

              <!-- Song info -->
              <div class="flex-1">
                <template v-if="getSongById(songId)">
                  <span class="text-sm text-white/80">{{ getSongById(songId)!.name }}</span>
                  <span class="text-xs text-white/40 ml-2">{{ getSongById(songId)!.bpm }} BPM</span>
                </template>
                <span v-else class="text-sm text-white/40 italic">Música removida</span>
              </div>

              <!-- Move buttons -->
              <button v-if="sIdx > 0"
                class="text-xs text-white/30 hover:text-white/70 px-1 cursor-pointer"
                @click="moveSong(sIdx, sIdx - 1)">↑</button>
              <button v-if="sIdx < editingSetlist.songs.length - 1"
                class="text-xs text-white/30 hover:text-white/70 px-1 cursor-pointer"
                @click="moveSong(sIdx, sIdx + 1)">↓</button>

              <!-- Remove -->
              <button class="text-xs text-red-400/40 hover:text-red-400 px-2 cursor-pointer opacity-0 group-hover:opacity-100 transition-opacity"
                      @click="removeSongFromSetlist(songId)">✕</button>
            </div>
          </div>
        </div>
      </div>
    </template>
  </div>
</template>
