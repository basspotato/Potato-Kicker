# Potato-Kicker — Project Brief

Read this first. Make small, targeted changes. Don't rewrite whole files unless asked.

## What it is
VST3 instrument: kick drum / bass generator focused on EDM.

## Repo & builds
- GitHub repo: `Potato-Kicker`
- Builds on GitHub Actions (same route as Magic Touch): Windows 11 VST3 + macOS version
- Framework: _[fill in, e.g. JUCE version]_
- Build files: _[fill in, e.g. CMakeLists.txt, .github/workflows/build.yml]_

## GUI
- First-run GUI approved, so build on it rather than redesigning.
- ADSR knobs: styled after the Magic Touch knobs (a new, integrated look, not a copy)
  - Attack = dark purple, Decay = blue, Sustain = orange, Release = yellow
  - Colour intensity rises as the knob turns right and falls as it turns left
- All non-ADSR knobs: white rings
- Display: waveform view next to COMP, styled after the Magic Touch display

## Working rules
- One feature per session.
- Show a short plan before big changes.
- Give edits/diffs, not full files.
- Developer compiles and pushes; share only relevant error lines when debugging.
- Keep DSP and GUI changes in separate sessions.
- Keep CPU low.

## Current status / next up
_[update at the end of each session: what's done, what's broken, what's next]_
