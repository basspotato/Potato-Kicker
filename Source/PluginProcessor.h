#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Params.h"
#include "Presets.h"
#include "KickEngine.h"

class PotatoKickerProcessor : public juce::AudioProcessor
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
    void setCurrentProgram (int index) override { loadPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Used by the editor
    void loadPreset (int index);
    bool presetIsModified() const;
    pk::EngineParams readParams() const;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;           // on-screen Trigger button feeds notes in here

    std::atomic<int>   currentPreset { 10 };          // Tech-House 01
    std::atomic<float> meterRms { 0.0f };
    std::atomic<float> meterGr { 0.0f };
    std::atomic<int>   heldNotes { 0 };
    std::atomic<uint32_t> noteOnCounter { 0 };       // editor watches this to animate the display playhead

private:
    pk::KickEngine engine;
    juce::Array<juce::RangedAudioParameter*> presetParams;
    struct Cached
    {
        std::atomic<float>* f[13];
        std::atomic<float>* wave; std::atomic<float>* comp; std::atomic<float>* stereo;
        std::atomic<float>* keytrack; std::atomic<float>* velocity;
    } raw {};
    int notesDown = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PotatoKickerProcessor)
};
