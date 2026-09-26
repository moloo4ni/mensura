# Mensura

A metronome DSP plugin for [fooyin](https://github.com/fooyin/fooyin): clicks are mixed into the playing
track, in time with its BPM tag or a tempo you set yourself.

## Requirements

- fooyin 0.13.1 with development headers (Arch: `fooyin`)
- Qt 6, CMake ≥ 3.19, a C++23 compiler

## Build and install

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build
cmake --install build
```

The plugin is installed to `~/.local/lib/fooyin/plugins/fyplugin_mensura.so`
(override with `-DMENSURA_PLUGIN_DIR=...`). Restart fooyin afterwards.

## Setup

1. **Settings → DSP**: add **Mensura** to the **per-track** chain. (In the per-track chain the clicks
   follow the track through crossfades.)
2. **View → Mensura** opens the control window. The metronome keeps working when the window is closed.

## Controls

- **Enable metronome** — on/off. Clicks only sound while a track is playing.
- **♩ BPM** — in **Auto** mode the track's `BPM` tag is used when present (label "tag"), otherwise the
  manual value (label "manual"). Editing the value switches to **Manual**.
- **TAP** (or the `T` key while the window has focus) — tap along with the music. The first tap of a
  series is the downbeat and the grid passes through your last tap. With a manual tempo, two or more taps
  also set the tempo (mean of the last 8 intervals); with a tag tempo only the phase moves. A pause of
  more than 2 s starts a new series.
- **Beats per bar** — 1–16; the first beat is accented (higher and louder). 1 = no accent.
- **Sound** — Click, Wood or Beep. **Volume** — −40…0 dB.
- **Phase offset** — fine-tune the grid by ±1000 ms; **Reset** clears the offset and the tapped phase.
  Changing the track also resets the phase.

If the window shows a warning that Mensura is not in the DSP chain, add the node as described above.

## Limitations

- During a crossfade both tracks' clicks are audible for its duration.
- There is no beat detection: without a BPM tag, set the tempo and phase by hand or with TAP.
