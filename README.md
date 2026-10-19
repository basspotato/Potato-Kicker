# Potato-Kicker

A kick drum and bass generator for EDM, built as a VST3 instrument for Windows 11 (x64) and as VST3 + Audio Unit for macOS 11 or later (Apple Silicon and Intel).

## Features

- **Oscillator:** Sine, Triangle, Saw, Square, Supersaw (7 detuned saws), Reese (2 beating detuned saws) and FM (2:1 growl that follows the envelope), with a Tune knob (30–120 Hz, shown with the note name).
- **Pitch envelope:** Pitch Amount (0–48 semitones) and Pitch Time (5–500 ms).
- **Amp envelope:** Attack, Decay, Sustain, Release.
  - Sustain at 0% makes each note a one-shot kick that always plays its full decay.
  - Raise Sustain and notes hold while the key is down, like an 808 or bass.
- **Character:** Click (noise and tone transient) and Drive (tanh saturation).
- **Compressor:** Threshold, Ratio, Makeup and an In/Out switch, plus a gain-reduction meter.
- **Output:** Volume, a Mono/Stereo switch (stereo widens the upper harmonics and keeps the low end mono), a soft output ceiling and a VU meter.
- **Look:** orange-and-yellow light-up console with backlit knob rings, an output level LED bar and the SPUD-01 potato emblem.
- **Display:** shows the rendered sound. Its Amp, Pitch and Comp tabs each have points you can drag with the mouse, and the Wave tab shows the live output as a scrolling, colour-coded waveform.
- **MIDI:** Key Track (C1 plays the Tune pitch, other notes transpose from there) and Velocity on/off. Works on any MIDI track, for example in Cubase.
- **100 factory presets:** 10 each for House, Tech-House, Techno, EDM, Festival, Progressive, Drum & Bass, Dubstep, Analog and Trap. They're also exposed to the DAW's program list.

## Getting the plugin

Every push to `main` builds the plugins with GitHub Actions.

### Windows

1. Open the repository's **Actions** tab and pick the latest **Build plugins** run.
2. Download the **Potato-Kicker-VST3-Windows-x64** artifact and unzip it.
3. Copy the `Potato-Kicker.vst3` folder to `C:\Program Files\Common Files\VST3\`.
4. Rescan plugins in your DAW. In Cubase, open **Studio > VST Plug-in Manager** and click **Rescan**.

### macOS

1. From the same run, download **Potato-Kicker-macOS-Universal**. It contains two zips: one for the VST3 and one for the Audio Unit.
2. Unzip them and copy the plugins into place:
   - `Potato-Kicker.vst3` → `~/Library/Audio/Plug-Ins/VST3/` (Cubase, Ableton Live, Bitwig, Studio One)
   - `Potato-Kicker.component` → `~/Library/Audio/Plug-Ins/Components/` (Logic Pro, GarageBand, Ableton Live)
3. The plugins are not notarized, so macOS blocks them the first time. In Terminal, clear the download flag:
   ```
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Potato-Kicker.vst3
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/Potato-Kicker.component
   ```
4. Restart or rescan in your DAW.

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
