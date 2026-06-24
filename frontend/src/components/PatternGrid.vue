<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue'

const props = defineProps<{
  grid: number[][]      // 14×16
  activeStep: number
  trackNames: string[]
  clipTrackGrid: number[] | null     // copied track data (16 steps) or null
}>()

const emit = defineEmits<{
  cycle: [track: number, step: number]
  copyTrack: [track: number]
  pasteTrack: [track: number]
  clearTrack: [track: number]
}>()

const beatLines = [4, 8, 12]

// Context menu state
const ctxMenu = ref<{ track: number; x: number; y: number } | null>(null)

function cellClass(vel: number): string {
  if (vel === 0) return 'bg-white/5 opacity-20'
  if (vel <= 0.4) return 'bg-indigo-400/30 opacity-40'
  if (vel <= 0.7) return 'bg-indigo-400/60 opacity-70'
  return 'bg-indigo-400 opacity-100 shadow-[0_0_8px_rgba(99,102,241,0.5)]'
}

function isActiveStep(step: number): boolean {
  return step === props.activeStep
}

// Right-click on track name → open context menu
function handleContextMenu(track: number, event: MouseEvent) {
  event.preventDefault()
  ctxMenu.value = { track, x: event.clientX, y: event.clientY }
}

function closeContextMenu() {
  ctxMenu.value = null
}

function doCopy() {
  if (ctxMenu.value) emit('copyTrack', ctxMenu.value.track)
  closeContextMenu()
}

function doPaste() {
  if (ctxMenu.value) emit('pasteTrack', ctxMenu.value.track)
  closeContextMenu()
}

function doClear() {
  if (ctxMenu.value) emit('clearTrack', ctxMenu.value.track)
  closeContextMenu()
}

// Close menu on click outside
function onGlobalClick() {
  closeContextMenu()
}
onMounted(() => document.addEventListener('click', onGlobalClick))
onUnmounted(() => document.removeEventListener('click', onGlobalClick))
</script>

<template>
  <div class="glass overflow-hidden relative">
    <!-- Header with step numbers -->
    <div class="flex border-b border-white/10">
      <div class="w-28 shrink-0 p-2 text-xs text-white/40 font-medium">Instrumento</div>
      <div class="flex flex-1">
        <div
          v-for="step in 16"
          :key="step"
          class="flex-1 p-2 text-center text-xs font-mono"
          :class="beatLines.includes(step - 1) ? 'border-l border-white/15' : ''"
        >
          <span :class="activeStep === step - 1 ? 'text-indigo-400 font-bold' : 'text-white/40'">
            {{ step }}
          </span>
        </div>
      </div>
    </div>

    <!-- Rows -->
    <div
      v-for="(track, tIdx) in grid"
      :key="tIdx"
      class="flex border-b border-white/5 transition-colors"
      :class="[tIdx % 2 === 0 ? 'bg-white/[0.02]' : '',
               ctxMenu?.track === tIdx ? 'bg-indigo-500/10' : 'hover:bg-white/5']"
    >
      <!-- Track name (right-click for context menu) -->
      <div
        class="w-28 shrink-0 p-2 text-sm truncate flex items-center border-r border-white/10 cursor-context-menu select-none"
        :class="clipTrackGrid ? 'text-indigo-300/80' : 'text-white/70'"
        @contextmenu="handleContextMenu(tIdx, $event)"
      >
        {{ trackNames[tIdx] || `Track ${tIdx + 1}` }}
      </div>

      <!-- Steps -->
      <div class="flex flex-1">
        <button
          v-for="(vel, sIdx) in track"
          :key="sIdx"
          class="flex-1 aspect-square flex items-center justify-center transition-all duration-150 cursor-pointer"
          :class="[
            beatLines.includes(sIdx) ? 'border-l border-white/10' : '',
            isActiveStep(sIdx) ? 'bg-white/5' : '',
          ]"
          @click="emit('cycle', tIdx, sIdx)"
        >
          <div
            class="w-5 h-5 rounded-sm transition-all duration-150"
            :class="cellClass(vel)"
          />
        </button>
      </div>
    </div>

    <!-- Context menu -->
    <div
      v-if="ctxMenu"
      class="fixed z-50 glass p-1 min-w-32"
      :style="{ left: ctxMenu.x + 'px', top: ctxMenu.y + 'px' }"
      @click.stop
    >
      <button class="block w-full text-left px-3 py-1.5 text-sm rounded-lg hover:bg-white/10 transition-colors text-white/80 cursor-pointer"
              @click="doCopy">
        Copiar track
      </button>
      <button class="block w-full text-left px-3 py-1.5 text-sm rounded-lg hover:bg-white/10 transition-colors text-white/80 cursor-pointer"
              :class="clipTrackGrid ? '' : 'opacity-30 pointer-events-none'"
              :disabled="!clipTrackGrid"
              @click="doPaste">
        Colar track
      </button>
      <div class="h-px bg-white/10 my-1" />
      <button class="block w-full text-left px-3 py-1.5 text-sm rounded-lg hover:bg-red-500/10 transition-colors text-red-400/60 cursor-pointer"
              @click="doClear">
        Limpar track
      </button>
    </div>
  </div>
</template>
