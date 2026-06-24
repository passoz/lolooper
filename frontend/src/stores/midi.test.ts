/**
 * Unit tests for the MIDI Pinia store.
 */
import { describe, it, expect, beforeEach, vi } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useMidiStore, type TransportState } from './midi'

// Mock WebMIDI API
const mockOutput = {
  id: 'output-1',
  name: 'Lolooper Virtual',
  send: vi.fn(),
}

const mockInput = {
  id: 'input-1',
  name: 'Lolooper Return',
  onmidimessage: null as ((event: MIDIMessageEvent) => void) | null,
}

const mockAccess = {
  outputs: new Map([['output-1', mockOutput as unknown as MIDIOutput]]),
  inputs: new Map([['input-1', mockInput as unknown as MIDIInput]]),
  onstatechange: null as (() => void) | null,
}

Object.defineProperty(navigator, 'requestMIDIAccess', {
  value: vi.fn().mockResolvedValue(mockAccess),
  writable: true,
})

describe('useMidiStore', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    vi.clearAllMocks()
  })

  it('initializes with disconnected state', () => {
    const store = useMidiStore()
    expect(store.connected).toBe(false)
    expect(store.outputs).toEqual([])
    expect(store.transportState.bar).toBe(0)
  })

  it('connect discovers MIDI ports', async () => {
    const store = useMidiStore()
    await store.connect()
    expect(store.connected).toBe(true)
    expect(store.outputs).toHaveLength(1)
    expect(store.outputs[0].name).toBe('Lolooper Virtual')
    expect(store.inputs).toHaveLength(1)
  })

  it('sendCC sends correct MIDI bytes', async () => {
    const store = useMidiStore()
    await store.connect()
    store.sendCC(15, 64)
    expect(mockOutput.send).toHaveBeenCalledWith([0xB0, 15, 64])
  })

  it('sendCC defaults to channel 0', async () => {
    const store = useMidiStore()
    await store.connect()
    store.sendCC(20, 100, 1)
    expect(mockOutput.send).toHaveBeenCalledWith([0xB1, 20, 100])
  })

  it('sendNote sends Note On followed by Note Off', async () => {
    vi.useFakeTimers()
    const store = useMidiStore()
    await store.connect()

    store.sendNote(60, 127)
    expect(mockOutput.send).toHaveBeenCalledWith([0x90, 60, 127])

    vi.advanceTimersByTime(50)
    expect(mockOutput.send).toHaveBeenCalledWith([0x80, 60, 0])
    vi.useRealTimers()
  })

  it('sendSysEx wraps data with 0xF0/0xF7 prefix', async () => {
    const store = useMidiStore()
    await store.connect()

    store.sendSysEx([0x03, 0x41, 0x42])
    expect(mockOutput.send).toHaveBeenCalledWith([0xF0, 0x7D, 0x03, 0x41, 0x42, 0xF7])
  })

  it('parses SysEx 0x05 transport state', async () => {
    const store = useMidiStore()
    await store.connect()

    // Simulate incoming SysEx: 0xF0 0x7D 0x05 bar beat step playing rec 0xF7
    const data = new Uint8Array([0xF0, 0x7D, 0x05, 12, 2, 4, 1, 0, 0xF7])
    const event = { data } as unknown as MIDIMessageEvent

    // Trigger the input handler
    mockInput.onmidimessage!(event)

    expect(store.transportState.bar).toBe(12)
    expect(store.transportState.beat).toBe(2)
    expect(store.transportState.step).toBe(4)
    expect(store.transportState.playing).toBe(true)
    expect(store.transportState.recording).toBe(false)
  })

  it('calls onTransportState callback when set', async () => {
    const store = useMidiStore()
    await store.connect()

    const callback = vi.fn()
    store.onTransportState = callback

    const data = new Uint8Array([0xF0, 0x7D, 0x05, 3, 1, 8, 0, 0, 0xF7])
    const event = { data } as unknown as MIDIMessageEvent
    mockInput.onmidimessage!(event)

    expect(callback).toHaveBeenCalledWith<[TransportState]>({
      bar: 3, beat: 1, step: 8, playing: false, recording: false,
    })
  })

  it('parses SysEx 0x06 song feedback', async () => {
    const store = useMidiStore()
    await store.connect()

    const callback = vi.fn()
    store.onSongFeedback = callback

    const data = new Uint8Array([0xF0, 0x7D, 0x06, 2, 4, 5, 0xF7])
    const event = { data } as unknown as MIDIMessageEvent
    mockInput.onmidimessage!(event)

    expect(callback).toHaveBeenCalledWith({ section: 2, barsDone: 4, totalSections: 5 })
  })
})
