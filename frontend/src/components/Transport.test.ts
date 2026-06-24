/**
 * Unit tests for the Transport component.
 */
import { describe, it, expect } from 'vitest'
import { mount } from '@vue/test-utils'
import Transport from './Transport.vue'

describe('Transport.vue', () => {
  it('renders play button when stopped', () => {
    const wrapper = mount(Transport, {
      props: {
        isPlaying: false,
        isRecording: false,
        bpm: 100,
        swing: 30,
        humanize: 10,
        currentBar: 0,
        currentBeat: 0,
        currentStep: 0,
      },
    })
    expect(wrapper.text()).toContain('▶')
  })

  it('renders pause button when playing', () => {
    const wrapper = mount(Transport, {
      props: {
        isPlaying: true,
        isRecording: false,
        bpm: 120,
        swing: 0,
        humanize: 0,
        currentBar: 3,
        currentBeat: 1,
        currentStep: 5,
      },
    })
    expect(wrapper.text()).toContain('⏸')
  })

  it('shows BPM value', () => {
    const wrapper = mount(Transport, {
      props: {
        isPlaying: false,
        isRecording: false,
        bpm: 140,
        swing: 0,
        humanize: 0,
      },
    })
    expect(wrapper.text()).toContain('140')
  })

  it('shows swing percentage', () => {
    const wrapper = mount(Transport, {
      props: {
        isPlaying: false,
        isRecording: false,
        bpm: 100,
        swing: 65,
        humanize: 0,
      },
    })
    expect(wrapper.text()).toContain('65%')
  })

  it('shows humanize in ms', () => {
    const wrapper = mount(Transport, {
      props: {
        isPlaying: false,
        isRecording: false,
        bpm: 100,
        swing: 0,
        humanize: 22,
      },
    })
    expect(wrapper.text()).toContain('22ms')
  })

  it('shows position indicator', () => {
    const wrapper = mount(Transport, {
      props: {
        isPlaying: true,
        isRecording: false,
        bpm: 100,
        swing: 0,
        humanize: 0,
        currentBar: 4,
        currentBeat: 2,
        currentStep: 10,
      },
    })
    expect(wrapper.text()).toContain('4.')
    expect(wrapper.text()).toContain('3.') // beat+1
    expect(wrapper.text()).toContain('11') // step+1
  })

  it('emits play when clicking play button', async () => {
    const wrapper = mount(Transport, {
      props: {
        isPlaying: false,
        isRecording: false,
        bpm: 100,
        swing: 0,
        humanize: 0,
      },
    })
    const playBtn = wrapper.find('button')
    await playBtn.trigger('click')
    expect(wrapper.emitted('play')).toBeTruthy()
  })

  it('emits swingChange when slider moves', async () => {
    const wrapper = mount(Transport, {
      props: {
        isPlaying: false,
        isRecording: false,
        bpm: 100,
        swing: 0,
        humanize: 0,
      },
    })
    const swingSlider = wrapper.findAll('input[type="range"]').at(0)
    if (swingSlider) {
      await swingSlider.setValue(50)
      expect(wrapper.emitted('swingChange')?.[0]).toEqual([50])
    }
  })

  it('emits humanizeChange when slider moves', async () => {
    const wrapper = mount(Transport, {
      props: {
        isPlaying: false,
        isRecording: false,
        bpm: 100,
        swing: 0,
        humanize: 0,
      },
    })
    const humanSlider = wrapper.findAll('input[type="range"]').at(1)
    if (humanSlider) {
      await humanSlider.setValue(25)
      expect(wrapper.emitted('humanizeChange')?.[0]).toEqual([25])
    }
  })
})
