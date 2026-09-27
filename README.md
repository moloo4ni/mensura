# Mensura

A metronome DSP plugin for [fooyin](https://github.com/fooyin/fooyin): clicks are mixed into the playing
track, in time with its BPM tag or a tempo you set yourself.

## Requirements

- fooyin 0.13.1 with development headers (Arch: `fooyin`)
- Qt 6, CMake ≥ 3.19, a C++23 compiler

## Install a release build

Each [release](https://github.com/moloo4ni/mensura/releases) has a prebuilt `fyplugin_mensura.so` for x86_64 Linux.
It is built on Arch Linux against fooyin 0.13.1 and Qt 6.11. For other fooyin or Qt versions, build from source.

```sh
tar -xzf mensura-1.0.0-x86_64.tar.gz
mkdir -p ~/.local/lib/fooyin/plugins
cp mensura-1.0.0-x86_64/fyplugin_mensura.so ~/.local/lib/fooyin/plugins/
```

Restart fooyin afterwards.

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

1. **Settings → Playback → DSP Manager**: add **Mensura** to the **Per-Track DSPs** chain.
   (In the per-track chain the clicks follow the track through crossfades.)
2. **View → Mensura** opens the control window. The metronome keeps working when the window is closed.

## Controls

- **Enable metronome** — on/off. Clicks only sound while a track is playing.
- **BPM** — in **Auto** mode the track's `BPM` tag is used when present (label "tag"), otherwise the
  manual value (label "manual"). Editing the value switches to **Manual**.
- **TAP** (or the `T` key while the window has focus) — tap along with the music. The first tap of a
  series is the downbeat and the grid passes through your last tap. With a manual tempo, two or more taps
  also set the tempo (mean of the last 8 intervals); with a tag tempo only the phase moves. A pause of
  more than 2 s starts a new series.
- **Beats per bar** — 1–16. 1 = no accent.
- **Accent** — how the first beat stands out: None, Pitch (higher pitched, same volume) or Pitch and volume
  (higher pitched, and the other beats are 6 dB quieter).
- **Sound** — Click, Wood, Beep or Mechanical. **Volume** — −40…0 dB.
- **Phase offset** — a standing correction of ±1000 ms (for example, for output latency). It is kept across
  track changes and restarts. Changing the track resets only the tapped phase; **Reset** clears both.

The mouse wheel changes a value only in a focused control, so scrolling over the window changes nothing.

If the window shows a warning that Mensura is not in the DSP chain, add the node as described above.

## Limitations

- During a crossfade both tracks' clicks are audible for its duration. After the switch the outgoing track
  clicks with the new track's tempo and phase, so two grids that do not line up can overlap until the fade ends.
- In gapless playback the first ~0.1–0.3 s of the next track may be clicked with the previous track's tempo and
  phase, because that audio is already processed before the track change reaches the plugin.
- Put Mensura first in the per-track chain, before any tempo, speed or resampling DSP. After such a DSP the audio
  no longer matches the track time, and the clicks may come out cut off.
- There is no beat detection: without a BPM tag, set the tempo and phase by hand or with TAP.

## License

Mensura is free software, licensed under the GNU General Public License v3.0 or later
(GPL-3.0-or-later), the same as fooyin. See [LICENSE](LICENSE).
