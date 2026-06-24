import { ref } from 'vue'
import { useMidiStore } from '../stores/midi'

export function useWebMIDI() {
  const midiStore = useMidiStore()
  const connected = ref(false)

  async function connect() {
    await midiStore.connect()
    connected.value = midiStore.connected
  }

  function isConnected(): boolean {
    return midiStore.connected
  }

  return {
    connected,
    connect,
    isConnected,
    sendCC: midiStore.sendCC,
    sendNote: midiStore.sendNote,
    sendSysEx: midiStore.sendSysEx,
    onMessage: midiStore.onMessage,
    transportState: midiStore.transportState,
    onTransportState: midiStore.onTransportState,
  }
}
