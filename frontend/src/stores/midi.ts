import { defineStore } from 'pinia'
import { ref, computed } from 'vue'

export interface TransportState {
  bar: number
  beat: number
  step: number
  playing: boolean
  recording: boolean
}

interface MidiState {
  outputs: MIDIOutput[]
  inputs: MIDIInput[]
  connected: boolean
}

export const useMidiStore = defineStore('midi', () => {
  const outputs = ref<MIDIOutput[]>([])
  const inputs = ref<MIDIInput[]>([])
  const connected = ref(false)
  const transportState = ref<TransportState>({
    bar: 0, beat: 0, step: 0, playing: false, recording: false,
  })

  // Callbacks for SysEx messages
  const onTransportState = ref<((state: TransportState) => void) | null>(null)
  const onSongFeedback = ref<((data: { section: number; barsDone: number; totalSections: number }) => void) | null>(null)

  let midiAccess: MIDIAccess | null = null

  function handleSysEx(data: Uint8Array) {
    if (data.length < 3 || data[0] !== 0x7D) return

    const cmd = data[1]

    // 0x05 = Transport state (sent every beat)
    if (cmd === 0x05 && data.length >= 7) {
      const state: TransportState = {
        bar: data[2],
        beat: data[3],
        step: data[4],
        playing: data[5] === 1,
        recording: data[6] === 1,
      }
      transportState.value = state
      if (onTransportState.value) onTransportState.value(state)
    }

    // 0x06 = Song feedback (sent every bar)
    // Payload: [0x7D, 0x06, section, barsDone, totalSections] = 5 bytes
    if (cmd === 0x06 && data.length >= 5) {
      const fb = { section: data[2], barsDone: data[3], totalSections: data[4] }
      if (onSongFeedback.value) onSongFeedback.value(fb)
    }
  }

  function setupMidiInput(input: MIDIInput) {
    input.onmidimessage = (event: MIDIMessageEvent) => {
      if (!event.data) return
      const bytes = new Uint8Array(event.data)

      // SysEx messages start with 0xF0
      if (bytes.length > 0 && bytes[0] === 0xF0) {
        // Strip 0xF0 (start) and 0xF7 (end), pass the payload
        const payload = bytes.slice(1, bytes[bytes.length - 1] === 0xF7 ? -1 : bytes.length)
        handleSysEx(payload)
      }
    }
  }

  async function connect() {
    if (!navigator.requestMIDIAccess) {
      console.warn('WebMIDI not supported in this browser')
      return
    }

    try {
      midiAccess = await navigator.requestMIDIAccess()
      outputs.value = [...midiAccess.outputs.values()]
      inputs.value = [...midiAccess.inputs.values()]
      connected.value = true

      // Set up input listeners
      inputs.value.forEach(setupMidiInput)

      midiAccess.onstatechange = () => {
        outputs.value = [...midiAccess!.outputs.values()]
        inputs.value = [...midiAccess!.inputs.values()]
        // Re-attach listeners to new inputs
        inputs.value.forEach(setupMidiInput)
        connected.value = midiAccess!.outputs.size > 0 || midiAccess!.inputs.size > 0
      }
    } catch (err) {
      console.error('Failed to connect MIDI:', err)
      connected.value = false
    }
  }

  function sendCC(cc: number, value: number, channel: number = 0) {
    const output = outputs.value[0]
    if (!output) return
    output.send([0xB0 | channel, cc, value])
  }

  function sendNote(note: number, velocity: number = 127) {
    const output = outputs.value[0]
    if (!output) return
    output.send([0x90, note, velocity])
    // Schedule Note Off after 50ms
    setTimeout(() => {
      output.send([0x80, note, 0])
    }, 50)
  }

  function sendSysEx(data: number[]) {
    const output = outputs.value[0]
    if (!output) return
    output.send([0xF0, 0x7D, ...data, 0xF7])
  }

  function onMessage(callback: (event: MIDIMessageEvent) => void) {
    inputs.value.forEach((input) => {
      input.onmidimessage = callback
    })
  }

  return {
    outputs,
    inputs,
    connected,
    transportState,
    onTransportState,
    onSongFeedback,
    connect,
    sendCC,
    sendNote,
    sendSysEx,
    onMessage,
  }
})
