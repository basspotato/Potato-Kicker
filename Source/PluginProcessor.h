#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Params.h"
#include "Presets.h"
#include "KickEngine.h"

class PotatoKickerProcessor : public juce::AudioProcessor,
                              private juce::AsyncUpdater
{
public:
    PotatoKickerProcessor();
    ~PotatoKickerProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Potato-Kicker"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return pk::numPresets; }
    int getCurrentProgram() override { return currentPreset.load(); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Used by the editor
    void loadPreset (int index, bool asUserGesture = true);
    bool presetIsModified() const;
    pk::EngineParams readParams() const;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;           // on-screen Trigger button feeds notes in here

    std::atomic<int>   currentPreset { 10 };          // Tech-House 01
    std::atomic<float> meterRms { 0.0f };
    std::atomic<float> meterGr { 0.0f };
    std::atomic<int>   heldNotes { 0 };
    std::atomic<uint32_t> noteOnCounter { 0 };       // editor watches this to animate the display playhead

    // Output scope for the display's WAVE view: the audio thread writes, the editor reads
    static constexpr int scopeSize = 1 << 15;
    int getScopeWritePos() const noexcept { return scopePos.load (std::memory_order_acquire); }
    void copyScope (float* dest, int numSamples, int endPos) const noexcept
    {
        const int start = (endPos - numSamples) & (scopeSize - 1);
        for (int i = 0; i < numSamples; ++i)
            dest[i] = scope[(size_t) ((start + i) & (scopeSize - 1))].load (std::memory_order_relaxed);
    }

private:
    pk::KickEngine engine;
    juce::Array<juce::RangedAudioParameter*> presetParams;
    struct Cached
    {
        std::atomic<float>* f[13];
        std::atomic<float>* wave; std::atomic<float>* comp; std::atomic<float>* stereo;
        std::atomic<float>* keytrack; std::atomic<float>* velocity;
    } raw {};
    void handleAsyncUpdate() override;
    std::atomic<int> pendingPreset { -1 };
    int notesDown = 0;
    std::array<std::atomic<float>, scopeSize> scope {};
    std::atomic<int> scopePos { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PotatoKickerProcessor)
};
