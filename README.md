# Potato-Kicker

A kick drum and bass generator for EDM, built as a VST3 instrument for Windows 11 (x64).

## Features

- **Oscillator:** Sine, Triangle, Saw and Square (band-limited), with a Tune knob (30–120 Hz, shown with the note name).
- **Pitch envelope:** Pitch Amount (0–48 semitones) and Pitch Time (5–500 ms).
- **Amp envelope:** Attack, Decay, Sustain, Release.
  - Sustain at 0% makes each note a one-shot kick that always plays its full decay.
  - Raise Sustain and notes hold while the key is down, like an 808 or bass.
- **Character:** Click (noise and tone transient) and Drive (tanh saturation).
- **Compressor:** Threshold, Ratio, Makeup and an In/Out switch, plus a gain-reduction meter.
- **Output:** Volume, a Mono/Stereo switch (stereo widens the upper harmonics and keeps the low end mono), a soft output ceiling and a VU meter.
- **Display:** shows the rendered sound. Its Amp, Pitch and Comp tabs each have points you can drag with the mouse.
- **MIDI:** Key Track (C1 plays the Tune pitch, other notes transpose from there) and Velocity on/off. Works on any MIDI track, for example in Cubase.
- **100 factory presets:** 10 each for House, Tech-House, Techno, EDM, Festival, Progressive, Drum & Bass, Dubstep, Analog and Trap. They're also exposed to the DAW's program list.

## Getting the plugin

Every push to `main` builds the plugin with GitHub Actions:

1. Open the repository's **Actions** tab and pick the latest **Build Windows VST3** run.
2. Download the **Potato-Kicker-VST3-Windows-x64** artifact and unzip it.
3. Copy the `Potato-Kicker.vst3` folder to `C:\Program Files\Common Files\VST3\`.
4. Rescan plugins in your DAW. In Cubase, open **Studio > VST Plug-in Manager** and click **Rescan**.

## Building locally (Windows)

Requirements: Visual Studio 2022 (Desktop C++ workload) and CMake 3.22+.

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target PotatoKicker_VST3
```

JUCE 8.0.4 is downloaded automatically. To use a local copy, pass `-DJUCE_DIR=path\to\JUCE`.

## Licenses

The fonts in `Resources/Fonts` are Barlow Condensed and Share Tech Mono, both under the SIL Open Font License (see the OFL files).
Built with [JUCE](https://juce.com).
