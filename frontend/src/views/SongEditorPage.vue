<script setup lang="ts">
import { ref, computed } from 'vue'
import { useSongsStore } from '../stores/songs'
import { useMidiStore } from '../stores/midi'

const songsStore = useSongsStore()
const midiStore = useMidiStore()

const showNewSong = ref(false)
const newSongName = ref('')
const newSongBpm = ref(100)
const newSongKey = ref('Cm')
const newSongStyle = ref('samba')

const editingSongId = ref<string | null>(null)
const editingSectionIndex = ref(-1)

const availableKeys = ['Cm', 'Dm', 'Em', 'Fm', 'Gm', 'Am', 'Bm',
                        'C', 'D', 'E', 'F', 'G', 'A', 'B',
                        'C#m', 'F#m', 'G#m']
const availableStyles = ref(['samba', 'pagode', 'partido_alto', 'samba_reggae',
                              'ijexa', 'frevo', 'maracatu', 'intro', 'virada'])

const editingSong = computed(() => {
  if (!editingSongId.value) return null
  return songsStore.songs.find(s => s.id === editingSongId.value) || null
})

function createSong() {
  if (!newSongName.value.trim()) return
  songsStore.addSong({
    id: crypto.randomUUID(),
    name: newSongName.value.trim(),
    bpm: newSongBpm.value,
    key: newSongKey.value,
    style: newSongStyle.value,
    sections: [],
  })
  newSongName.value = ''
  showNewSong.value = false
}

function editSong(id: string) {
  editingSongId.value = id
  editingSectionIndex.value = -1
}

function closeEditor() {
  editingSongId.value = null
  editingSectionIndex.value = -1
}

// Section management
function addSection() {
  if (!editingSong.value) return
  const song = editingSong.value
  song.sections.push({
    name: `Seção ${song.sections.length + 1}`,
    patternStyle: song.style,
    bars: 8,
    actions: [],
  })
  songsStore.updateSong(song.id, { sections: [...song.sections] })
  editingSectionIndex.value = song.sections.length - 1
}

function removeSection(index: number) {
  if (!editingSong.value) return
  editingSong.value.sections.splice(index, 1)
  songsStore.updateSong(editingSong.value.id, { sections: [...editingSong.value.sections] })
  if (editingSectionIndex.value === index) editingSectionIndex.value = -1
}

function moveSection(from: number, to: number) {
  if (!editingSong.value) return
  const sections = [...editingSong.value.sections]
  const [removed] = sections.splice(from, 1)
  sections.splice(to, 0, removed)
  editingSong.value.sections = sections
  songsStore.updateSong(editingSong.value.id, { sections })
}

function selectEditingSection(index: number) {
  editingSectionIndex.value = editingSectionIndex.value === index ? -1 : index
}

function updateSection(index: number, field: string, value: unknown) {
  if (!editingSong.value) return
  const sections = [...editingSong.value.sections]
  ;(sections[index] as Record<string, unknown>)[field] = value
  editingSong.value.sections = sections
  songsStore.updateSong(editingSong.value.id, { sections })
}

/** Update a specific action within a section. */
function updateAction(sectionIdx: number, actionIdx: number, field: string, value: unknown) {
  if (!editingSong.value) return
  const sections = [...editingSong.value.sections]
  const actions = [...sections[sectionIdx].actions]
  ;(actions[actionIdx] as Record<string, unknown>)[field] = value
  sections[sectionIdx].actions = actions
  editingSong.value.sections = sections
  songsStore.updateSong(editingSong.value.id, { sections })
}

// Actions within a section
function addAction(sectionIndex: number) {
  if (!editingSong.value) return
  const sections = [...editingSong.value.sections]
  sections[sectionIndex].actions.push({ atBar: 0, cmd: 'mute', track: '' })
  editingSong.value.sections = sections
  songsStore.updateSong(editingSong.value.id, { sections })
}

function removeAction(sectionIndex: number, actionIndex: number) {
  if (!editingSong.value) return
  const sections = [...editingSong.value.sections]
  sections[sectionIndex].actions.splice(actionIndex, 1)
  editingSong.value.sections = sections
  songsStore.updateSong(editingSong.value.id, { sections })
}

// Export song as JSON file
function exportSong() {
  if (!editingSong.value) return
  const blob = new Blob([JSON.stringify(editingSong.value, null, 2)], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = `${editingSong.value.name.replace(/\s+/g, '_')}.json`
  a.click()
  URL.revokeObjectURL(url)
}

// Import song from JSON file
function importSong(file: File) {
  file.text().then(text => {
    try {
      const song = JSON.parse(text)
      if (!song.name || !song.bpm) throw new Error('Invalid song format')
      song.id = crypto.randomUUID()
      if (!song.sections) song.sections = []
      songsStore.addSong(song)
    } catch (e) {
      console.error('Import failed:', e)
    }
  }).catch(e => console.error('File read error:', e))
}

// Send song structure to plugin
function sendSongToPlugin() {
  if (!editingSong.value) return
  const data = JSON.stringify({
    name: editingSong.value.name,
    bpm: editingSong.value.bpm,
    sections: editingSong.value.sections.map(s => ({
      name: s.name,
      pattern: s.patternStyle,
      bars: s.bars,
      loop: false,
    })),
  })
  midiStore.sendSysEx([0x04, ...new TextEncoder().encode(data)])
}

// Update song metadata
function updateSongField(field: string, value: unknown) {
  if (!editingSong.value) return
  songsStore.updateSong(editingSong.value.id, { [field]: value })
}
</script>

<template>
  <div class="space-y-4">
    <div class="flex items-center justify-between">
      <h2 class="text-lg font-bold">Músicas</h2>
      <button
        class="px-4 py-2 glass text-sm cursor-pointer hover:bg-white/10 transition-colors"
        @click="showNewSong = !showNewSong"
      >
        + Nova Música
      </button>
    </div>

    <!-- New song form (modal) -->
    <div v-if="showNewSong" class="glass p-4 space-y-3">
      <input
        v-model="newSongName"
        type="text"
        placeholder="Nome da música"
        class="w-full px-3 py-2 bg-white/10 border border-white/20 rounded-lg text-white placeholder-white/30 outline-none focus:border-indigo-500/50"
        @keyup.enter="createSong"
      />
      <div class="flex flex-wrap gap-3 items-center">
        <div class="flex items-center gap-2">
          <span class="text-xs text-white/40">BPM</span>
          <input v-model.number="newSongBpm" type="number" min="20" max="300"
            class="w-16 px-2 py-1 bg-white/10 border border-white/20 rounded text-center text-sm font-mono text-white" />
        </div>
        <div class="flex items-center gap-2">
          <span class="text-xs text-white/40">Tom</span>
          <select v-model="newSongKey"
            class="px-2 py-1 bg-white/10 border border-white/20 rounded text-sm text-white outline-none">
            <option v-for="k in availableKeys" :key="k" :value="k">{{ k }}</option>
          </select>
        </div>
        <div class="flex items-center gap-2">
          <span class="text-xs text-white/40">Estilo</span>
          <select v-model="newSongStyle"
            class="px-2 py-1 bg-white/10 border border-white/20 rounded text-sm text-white outline-none">
            <option v-for="s in availableStyles" :key="s" :value="s">{{ s }}</option>
          </select>
        </div>
        <button class="px-4 py-1.5 bg-indigo-500/30 text-indigo-300 rounded-lg text-sm
                       hover:bg-indigo-500/50 transition-colors cursor-pointer"
                @click="createSong">Criar</button>
      </div>
    </div>

    <!-- Song list or editor -->
    <template v-if="!editingSongId">
      <!-- List view -->
      <div v-if="songsStore.songs.length === 0" class="glass p-8 text-center text-white/40">
        <p class="text-4xl mb-4">🎵</p>
        <p>Nenhuma música ainda. Crie a primeira!</p>
      </div>

      <div v-else class="space-y-2">
        <div v-for="song in songsStore.songs" :key="song.id"
          class="glass p-4 flex items-center justify-between hover:bg-white/[0.07] transition-colors cursor-pointer"
          @click="editSong(song.id)">
          <div>
            <h3 class="font-medium">{{ song.name }}</h3>
            <p class="text-sm text-white/40">
              {{ song.bpm }} BPM · {{ song.key }} · {{ song.style }} · {{ song.sections.length }} seções
            </p>
          </div>
          <div class="flex gap-2">
            <button class="px-3 py-1 text-xs glass hover:bg-white/10 transition-colors cursor-pointer"
                    @click.stop="editSong(song.id)">Editar</button>
            <button class="px-3 py-1 text-xs text-red-400/60 hover:text-red-400
                           hover:bg-red-500/10 rounded-lg transition-colors cursor-pointer"
                    @click.stop="songsStore.removeSong(song.id)">Remover</button>
          </div>
        </div>
      </div>
    </template>

    <!-- Song Editor (detail view) -->
    <template v-else-if="editingSong">
      <div class="glass p-4 space-y-4">
        <!-- Header -->
        <div class="flex items-center justify-between">
          <div class="flex items-center gap-3 flex-1">
            <input :value="editingSong.name"
              @input="updateSongField('name', ($event.target as HTMLInputElement).value)"
              class="px-3 py-1.5 bg-white/10 border border-white/20 rounded-lg text-white text-lg font-bold outline-none focus:border-indigo-500/50 w-64" />
          </div>
          <div class="flex gap-2">
            <button class="px-3 py-1.5 text-xs glass text-indigo-300 hover:bg-indigo-500/20
                           transition-colors cursor-pointer"
                    @click="sendSongToPlugin">
              Enviar ao Plugin
            </button>
            <button class="px-3 py-1.5 text-xs glass text-white/60 hover:text-white/80
                           transition-colors cursor-pointer"
                    @click="exportSong">
              Exportar
            </button>
            <label class="px-3 py-1.5 text-xs glass text-white/60 hover:text-white/80
                          cursor-pointer inline-flex items-center gap-1">
              Importar
              <input type="file" accept=".json" class="hidden"
                @change="(e) => { const file = (e.target as HTMLInputElement).files?.[0]; if (file) importSong(file); }" />
            </label>
            <button class="px-3 py-1.5 text-xs glass text-white/60 hover:text-white/80
                           transition-colors cursor-pointer"
                    @click="closeEditor">Voltar</button>
          </div>
        </div>

        <!-- Song metadata -->
        <div class="flex flex-wrap gap-4 items-center">
          <div class="flex items-center gap-2">
            <span class="text-xs text-white/40">BPM</span>
            <input :value="editingSong.bpm"
              @input="updateSongField('bpm', Number(($event.target as HTMLInputElement).value))"
              type="number" min="20" max="300"
              class="w-16 px-2 py-1 bg-white/10 border border-white/20 rounded text-center text-sm font-mono text-white" />
          </div>
          <div class="flex items-center gap-2">
            <span class="text-xs text-white/40">Tom</span>
            <select :value="editingSong.key"
              @change="updateSongField('key', ($event.target as HTMLSelectElement).value)"
              class="px-2 py-1 bg-white/10 border border-white/20 rounded text-sm text-white outline-none">
              <option v-for="k in availableKeys" :key="k" :value="k">{{ k }}</option>
            </select>
          </div>
          <div class="flex items-center gap-2">
            <span class="text-xs text-white/40">Estilo</span>
            <select :value="editingSong.style"
              @change="updateSongField('style', ($event.target as HTMLSelectElement).value)"
              class="px-2 py-1 bg-white/10 border border-white/20 rounded text-sm text-white outline-none">
              <option v-for="s in availableStyles" :key="s" :value="s">{{ s }}</option>
            </select>
          </div>
        </div>

        <!-- Sections -->
        <div>
          <div class="flex items-center justify-between mb-2">
            <h3 class="text-sm font-medium text-white/60 uppercase tracking-wider">Seções ({{ editingSong.sections.length }})</h3>
            <button class="px-3 py-1 text-xs glass text-indigo-300 hover:bg-indigo-500/20
                           transition-colors cursor-pointer"
                    @click="addSection">+ Seção</button>
          </div>

          <div v-if="editingSong.sections.length === 0" class="text-sm text-white/40 py-4 text-center">
            Nenhuma seção. Adicione uma seção para definir a estrutura da música.
          </div>

          <div v-else class="space-y-2">
            <div v-for="(section, sIdx) in editingSong.sections" :key="sIdx"
              class="glass p-3 space-y-2">
              <div class="flex items-center gap-2">
                <!-- Drag handle -->
                <span class="text-white/20 cursor-grab text-sm">⠿</span>

                <!-- Section name -->
                <input :value="section.name"
                  @input="updateSection(sIdx, 'name', ($event.target as HTMLInputElement).value)"
                  class="flex-1 px-2 py-1 bg-white/10 border border-white/20 rounded text-sm text-white outline-none focus:border-indigo-500/50" />

                <!-- Pattern -->
                <select :value="section.patternStyle"
                  @change="updateSection(sIdx, 'patternStyle', ($event.target as HTMLSelectElement).value)"
                  class="px-2 py-1 bg-white/10 border border-white/20 rounded text-sm text-white outline-none">
                  <option v-for="s in availableStyles" :key="s" :value="s">{{ s }}</option>
                </select>

                <!-- Bars -->
                <div class="flex items-center gap-1">
                  <span class="text-xs text-white/40">compassos</span>
                  <input :value="section.bars"
                    @input="updateSection(sIdx, 'bars', Number(($event.target as HTMLInputElement).value))"
                    type="number" min="1" max="64"
                    class="w-12 px-1 py-1 bg-white/10 border border-white/20 rounded text-center text-sm font-mono text-white" />
                </div>

                <!-- Action buttons -->
                <button class="px-2 py-1 text-xs text-white/40 hover:text-white/70
                               hover:bg-white/10 rounded cursor-pointer"
                        @click="addAction(sIdx)">+ Ação</button>
                <button class="px-2 py-1 text-xs text-red-400/60 hover:text-red-400
                               hover:bg-red-500/10 rounded cursor-pointer"
                        @click="removeSection(sIdx)">✕</button>
              </div>

              <!-- Section actions (collapsible) -->
              <div v-if="section.actions.length > 0 && editingSectionIndex === sIdx" class="ml-6 space-y-1">
                <div v-for="(action, aIdx) in section.actions" :key="aIdx"
                  class="flex items-center gap-2 text-xs text-white/60">
                  <span>No compasso</span>
                  <input :value="action.atBar"
                    @input="updateAction(sIdx, aIdx, 'atBar', Number(($event.target as HTMLInputElement).value))"
                    type="number" min="0" class="w-10 px-1 py-0.5 bg-white/10 border border-white/20 rounded text-center font-mono text-white" />
                  <select :value="action.cmd"
                    @change="updateAction(sIdx, aIdx, 'cmd', ($event.target as HTMLSelectElement).value)"
                    class="px-1 py-0.5 bg-white/10 border border-white/20 rounded text-white outline-none">
                    <option value="mute">Mutar</option>
                    <option value="unmute">Ativar</option>
                    <option value="set_accent">Accent</option>
                    <option value="fade_out">Fade out</option>
                  </select>
                  <input v-if="action.cmd !== 'fade_out'" :value="action.track || ''"
                    @input="updateAction(sIdx, aIdx, 'track', ($event.target as HTMLInputElement).value)"
                    placeholder="instrumento"
                    class="w-24 px-1 py-0.5 bg-white/10 border border-white/20 rounded text-white outline-none" />
                  <button class="text-red-400/60 hover:text-red-400 cursor-pointer"
                          @click="removeAction(sIdx, aIdx)">✕</button>
                </div>
              </div>
            </div>
          </div>
        </div>

        <!-- Timeline preview -->
        <div v-if="editingSong.sections.length > 0" class="glass p-3">
          <h4 class="text-xs text-white/40 uppercase tracking-wider mb-2">Linha do tempo</h4>
          <div class="flex gap-1 items-end h-20">
            <div v-for="(section, sIdx) in editingSong.sections" :key="sIdx"
              class="flex-1 rounded-t transition-all duration-200 cursor-pointer hover:opacity-80 relative group"
              :style="{ height: Math.max(20, section.bars * 5) + 'px' }"
              :class="sIdx === editingSectionIndex ? 'bg-indigo-500/50 border border-indigo-400/50' : 'bg-white/10 border border-white/10'"
              @click="selectEditingSection(sIdx)">
              <span class="absolute -top-4 left-1/2 -translate-x-1/2 text-[10px] text-white/40 whitespace-nowrap opacity-0 group-hover:opacity-100">
                {{ section.name }}
              </span>
            </div>
          </div>
        </div>
      </div>
    </template>
  </div>
</template>
