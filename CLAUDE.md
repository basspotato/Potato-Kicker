# Potato-Kicker

VST3 + AU kick drum / bass generator for EDM. JUCE 8.0.15, C++17.
Targets: Windows 11 x64 (VST3), macOS 11+ Universal (VST3 + AU).

## Build

```bash
# macOS
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target PotatoKicker_VST3

# Windows (Visual Studio 2022)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target PotatoKicker_VST3
```

To use a local JUCE checkout: `-DJUCE_DIR=path/to/JUCE`.
Releases are built by GitHub Actions (`.github/workflows/build.yml`) — download from the run's Artifacts tab.

## Source layout

```
Source/
  KickEngine.h        — DSP: oscillator, pitch envelope, amp envelope, drive, compressor
  Params.h            — all APVTS parameter IDs and ranges
  Presets.h           — 100 factory presets (10 per genre)
  PluginProcessor.*   — audio thread, parameter handling
  PluginEditor.*      — GUI
Resources/Fonts/      — Barlow Condensed, Share Tech Mono (binary data via juce_add_binary_data)
```

## Architecture notes

- All synthesis lives in `KickEngine.h`. `PluginProcessor` owns one instance and calls it from `processBlock`.
- Parameters are declared in `Params.h` and stored in an `AudioProcessorValueTreeState`. Don't reach into the APVTS from the audio thread — use atomic parameters.
- Sustain at 0 → one-shot kick (always plays full decay). Sustain > 0 → note holds while key is down (808/bass mode). This is intentional.
- Presets are not exposed as host programs — selecting a track or sending MIDI program changes must never replace the user's sound.

## GUI conventions

- ADSR knobs: Attack = dark purple, Decay = blue, Sustain = orange, Release = yellow. Ring colour intensity rises/falls with knob value.
- All non-ADSR knobs: white rings.
- Display panel tabs: Amp, Pitch, Comp (draggable envelope points), Wave (scrolling waveform).
- Orange-and-yellow "light-up console" aesthetic. The first-run GUI is approved — build on it, don't redesign.

## Working rules

- One feature per session.
- Show a short plan before any big change.
- Give diffs/edits, not full file rewrites — the developer compiles and pushes.
- When debugging a build error, share only the relevant error lines.
- Keep DSP and GUI changes in separate sessions.
- Keep CPU low. Don't add cost (extra allocations, high repaint rates, redundant computation) without flagging it.
- Never allocate on the audio thread.
