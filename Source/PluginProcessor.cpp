#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Order matches the float fields of pk::Preset after `wave`
    const char* const floatIds[13] = { pk::id::tune, pk::id::pamt, pk::id::pdec, pk::id::atk, pk::id::dec, pk::id::sus,
                                       pk::id::rel, pk::id::click, pk::id::drive, pk::id::thr, pk::id::ratio,
                                       pk::id::makeup, pk::id::vol };

    float presetValue (const pk::Preset& p, int i)
    {
        const float v[13] = { p.tune, p.pamt, p.pdec, p.atk, p.dec, p.sus, p.rel, p.click, p.drive, p.thr, p.ratio, p.makeup, p.vol };
        return v[i];
    }
}

PotatoKickerProcessor::PotatoKickerProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PotatoKicker", pk::createLayout())
{
    for (int i = 0; i < 13; ++i)
    {
        raw.f[i] = apvts.getRawParameterValue (floatIds[i]);
        presetParams.add (apvts.getParameter (floatIds[i]));
    }
    raw.wave     = apvts.getRawParameterValue (pk::id::wave);
    raw.comp     = apvts.getRawParameterValue (pk::id::comp);
    raw.stereo   = apvts.getRawParameterValue (pk::id::stereo);
    raw.keytrack = apvts.getRawParameterValue (pk::id::keytrack);
    raw.velocity = apvts.getRawParameterValue (pk::id::velocity);

    loadPreset (currentPreset.load());
}

bool PotatoKickerProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

pk::EngineParams PotatoKickerProcessor::readParams() const
{
    pk::EngineParams p;
    p.tune = raw.f[0]->load(); p.pamt = raw.f[1]->load(); p.pdec = raw.f[2]->load();
    p.atk = raw.f[3]->load();  p.dec = raw.f[4]->load();  p.sus = raw.f[5]->load();  p.rel = raw.f[6]->load();
    p.click = raw.f[7]->load(); p.drive = raw.f[8]->load();
    p.thr = raw.f[9]->load();  p.ratio = raw.f[10]->load(); p.makeup = raw.f[11]->load(); p.vol = raw.f[12]->load();
    p.wave = juce::jlimit (0, 3, (int) raw.wave->load());
    p.comp = raw.comp->load() > 0.5f;
    p.stereo = raw.stereo->load() > 0.5f;
    p.keytrack = raw.keytrack->load() > 0.5f;
    p.velocity = raw.velocity->load() > 0.5f;
    return p;
}

void PotatoKickerProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate, (uint32_t) juce::Random::getSystemRandom().nextInt() | 1u);
    engine.setParams (readParams());
    keyboardState.reset();
    notesDown = 0;
    heldNotes = 0;
}

void PotatoKickerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    engine.setParams (readParams());

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, numSamples);

    float sumSq = 0.0f, gr = 0.0f;
    int pos = 0;
    auto renderTo = [&] (int end)
    {
        if (end > pos)
            engine.render (L + pos, R != nullptr ? R + pos : nullptr, end - pos, sumSq, gr);
        pos = end;
    };

    for (const auto meta : midi)
    {
        renderTo (juce::jlimit (pos, numSamples, meta.samplePosition));
        const auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            engine.noteOn (m.getNoteNumber(), m.getVelocity());
            ++notesDown;
            noteOnCounter.fetch_add (1);
        }
        else if (m.isNoteOff())
        {
            engine.noteOff (m.getNoteNumber());
            notesDown = juce::jmax (0, notesDown - 1);
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            engine.allNotesOff();
            notesDown = 0;
        }
    }
    renderTo (numSamples);

    heldNotes = notesDown;
    meterRms = std::sqrt (sumSq / (float) juce::jmax (1, numSamples));
    meterGr = gr;
}

void PotatoKickerProcessor::loadPreset (int index)
{
    index = juce::jlimit (0, pk::numPresets - 1, index);
    const auto& p = pk::presets[index];
    currentPreset = index;

    auto set = [] (juce::RangedAudioParameter* prm, float value)
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (value));
        prm->endChangeGesture();
    };
    set (apvts.getParameter (pk::id::wave), (float) p.wave);
    for (int i = 0; i < 13; ++i)
        set (presetParams[i], presetValue (p, i));
}

bool PotatoKickerProcessor::presetIsModified() const
{
    const auto& p = pk::presets[currentPreset.load()];
    if ((int) raw.wave->load() != p.wave) return true;
    for (int i = 0; i < 13; ++i)
    {
        const auto* prm = presetParams[i];
        if (std::abs (prm->convertTo0to1 (raw.f[i]->load()) - prm->convertTo0to1 (presetValue (p, i))) > 0.002f)
            return true;
    }
    return false;
}

const juce::String PotatoKickerProcessor::getProgramName (int index)
{
    index = juce::jlimit (0, pk::numPresets - 1, index);
    return juce::String (pk::genreNames[index / pk::presetsPerGenre]) + " - " + pk::presets[index].name;
}

void PotatoKickerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset.load(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void PotatoKickerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentPreset = juce::jlimit (0, pk::numPresets - 1, (int) tree.getProperty ("preset", 10));
            apvts.replaceState (tree);
        }
    }
}

juce::AudioProcessorEditor* PotatoKickerProcessor::createEditor()
{
    return new PotatoKickerEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PotatoKickerProcessor();
}
