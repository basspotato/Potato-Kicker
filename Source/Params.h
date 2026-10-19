#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>

namespace pk
{
    // Parameter IDs (also used as preset column order, see Presets.h)
    namespace id
    {
        inline constexpr const char* wave     = "wave";
        inline constexpr const char* tune     = "tune";
        inline constexpr const char* pamt     = "pamt";
        inline constexpr const char* pdec     = "pdec";
        inline constexpr const char* atk      = "atk";
        inline constexpr const char* dec      = "dec";
        inline constexpr const char* sus      = "sus";
        inline constexpr const char* rel      = "rel";
        inline constexpr const char* click    = "click";
        inline constexpr const char* drive    = "drive";
        inline constexpr const char* thr      = "thr";
        inline constexpr const char* ratio    = "ratio";
        inline constexpr const char* makeup   = "makeup";
        inline constexpr const char* vol      = "vol";
        inline constexpr const char* comp     = "comp";
        inline constexpr const char* stereo   = "stereo";
        inline constexpr const char* keytrack = "keytrack";
        inline constexpr const char* velocity = "velocity";
    }

    inline const juce::StringArray waveNames { "SINE", "TRI", "SAW", "SQR" };

    inline juce::String noteName (float hz)
    {
        static const char* nn[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
        const int m = (int) std::lround (69.0 + 12.0 * std::log2 (hz / 440.0));
        return juce::String (nn[((m % 12) + 12) % 12]) + juce::String ((int) std::floor (m / 12.0) - 1);
    }

    inline juce::String fmtMs (float v)
    {
        if (v < 10.0f)   return juce::String (v, 1) + " ms";
        if (v < 1000.0f) return juce::String (juce::roundToInt (v)) + " ms";
        return juce::String (v / 1000.0f, 2) + " s";
    }

    inline juce::String formatValue (const juce::String& pid, float v)
    {
        if (pid == id::tune)   return juce::String (v, 1) + " Hz · " + noteName (v);
        if (pid == id::pamt)   return "+" + juce::String (juce::roundToInt (v)) + " st";
        if (pid == id::pdec || pid == id::atk || pid == id::dec || pid == id::rel) return fmtMs (v);
        if (pid == id::click || pid == id::drive || pid == id::sus) return juce::String (juce::roundToInt (v)) + "%";
        if (pid == id::thr)    return juce::String (v, 1) + " dB";
        if (pid == id::ratio)  return juce::String (v, 1) + ":1";
        if (pid == id::makeup) return "+" + juce::String (v, 1) + " dB";
        if (pid == id::vol)    return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB";
        return juce::String (v, 2);
    }

    inline juce::NormalisableRange<float> logRange (float lo, float hi)
    {
        return { lo, hi,
                 [] (float s, float e, float n) { return s * std::pow (e / s, n); },
                 [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); },
                 [] (float s, float e, float v) { return juce::jlimit (s, e, v); } };
    }

    inline juce::NormalisableRange<float> cubeRange (float lo, float hi)
    {
        return { lo, hi,
                 [] (float s, float e, float n) { return s + (e - s) * n * n * n; },
                 [] (float s, float e, float v) { return std::cbrt (juce::jmax (0.0f, (v - s) / (e - s))); },
                 [] (float s, float e, float v) { return juce::jlimit (s, e, v); } };
    }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using namespace juce;
        AudioProcessorValueTreeState::ParameterLayout layout;

        auto addF = [&] (const char* pid, const char* name, NormalisableRange<float> r, float def, const char* unit)
        {
            const String p (pid);
            layout.add (std::make_unique<AudioParameterFloat> (ParameterID { pid, 1 }, name, r, def,
                AudioParameterFloatAttributes()
                    .withLabel (unit)
                    .withStringFromValueFunction ([p] (float v, int) { return formatValue (p, v); })));
        };

        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id::wave, 1 }, "Waveform", waveNames, 0));
        addF (id::tune,   "Tune",          logRange (30.0f, 120.0f),   52.0f, "Hz");
        addF (id::pamt,   "Pitch Amount",  NormalisableRange<float> (0.0f, 48.0f), 30.0f, "st");
        addF (id::pdec,   "Pitch Time",    logRange (5.0f, 500.0f),    30.0f, "ms");
        addF (id::atk,    "Attack",        cubeRange (0.0f, 100.0f),   0.0f,  "ms");
        addF (id::dec,    "Decay",         logRange (10.0f, 3000.0f),  240.0f, "ms");
        addF (id::sus,    "Sustain",       NormalisableRange<float> (0.0f, 100.0f), 0.0f, "%");
        addF (id::rel,    "Release",       logRange (5.0f, 3000.0f),   40.0f, "ms");
        addF (id::click,  "Click",         NormalisableRange<float> (0.0f, 100.0f), 50.0f, "%");
        addF (id::drive,  "Drive",         NormalisableRange<float> (0.0f, 100.0f), 25.0f, "%");
        addF (id::thr,    "Comp Threshold", NormalisableRange<float> (-40.0f, 0.0f), -14.0f, "dB");
        addF (id::ratio,  "Comp Ratio",    logRange (1.0f, 20.0f),     5.0f,  ":1");
        addF (id::makeup, "Comp Makeup",   NormalisableRange<float> (0.0f, 18.0f), 4.0f, "dB");
        addF (id::vol,    "Volume",        NormalisableRange<float> (-36.0f, 6.0f), -3.0f, "dB");
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id::comp, 1 },     "Comp On",   true));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id::stereo, 1 },   "Stereo Output", false,
                                                          AudioParameterBoolAttributes().withStringFromValueFunction ([] (bool b, int) { return b ? "Stereo" : "Mono"; })));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id::keytrack, 1 }, "Key Track", true));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id::velocity, 1 }, "Velocity",  true));
        return layout;
    }
}
