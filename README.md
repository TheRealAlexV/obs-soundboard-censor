# OBS Soundboard & Censor Plugin

An OBS Studio plugin that provides a **Censorship Audio Filter** (push-to-censor with bleep audio) and a **Soundboard** dock widget.

## Features

### Censorship Audio Filter
- Apply as an audio filter to any audio source (e.g., microphone)
- Register a "Hold to Censor" hotkey
- When held: mutes the source audio and plays a configurable bleep sound on loop
- Configurable bleep audio file (MP3, WAV, OGG, FLAC, etc.)
- Adjustable bleep volume

### Soundboard (coming soon)
- Dockable Qt widget with play/stop buttons and volume sliders
- Per-sound hotkey assignment
- Settings persistence across sessions

## Building

### Prerequisites
- OBS Studio 31.x SDK (obs-deps)
- CMake 3.28+
- Visual Studio 2022 (Windows) or Clang (macOS)
- Qt 6

### Windows Build

```powershell
# Set up OBS dependencies (requires obs-deps and obs-studio source)
cmake --preset windows-x64
cmake --build build_x64 --config RelWithDebInfo --parallel
```

### Manual Configure (Windows)

```powershell
cmake -S . -B build_x64 -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_PREFIX_PATH=".deps;.deps/obs-deps-VERSION-x64;.deps/obs-deps-qt6-VERSION-x64"
cmake --build build_x64 --config RelWithDebInfo --parallel
```

## Installation

1. Copy `obs-soundboard-censor.dll` to `<OBS>/obs-plugins/64bit/`
2. Copy `data/` to `<OBS>/data/obs-plugins/obs-soundboard-censor/`

## Usage

### Censor Filter
1. Select an audio source in OBS (e.g., your microphone)
2. Add filter → "Censorship Filter"
3. Select a bleep audio file
4. Set bleep volume
5. Go to Settings → Hotkeys and bind "Hold to Censor" 
6. Hold the hotkey to mute the mic and play the bleep

## License

MIT
