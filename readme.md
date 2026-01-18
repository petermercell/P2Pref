# P2Pref

A C++ plugin for Foundry's Nuke that converts a **P (position) pass** to a **Pref (reference position) pass** using an Axis node.

## Overview

P2Pref solves a common VFX challenge: projecting textures onto animated or deforming geometry with stable, non-sliding results. By transforming world-space position data (P pass) into reference-space coordinates (Pref pass) using a tracked Axis, you can create stable UV coordinates for texture projection workflows.

### Workflow

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   P Pass    │───▶│   P2Pref    │───▶│ PRefToMotion│───▶│  STMap /    │
│ (from 3D)   │    │ + Axis/Loc  │    │             │    │  Projection │
└─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘
```

P2Pref is designed to work seamlessly with [PRefToMotion](https://github.com/petermercell/PRefToMotion), which converts the Pref pass into UV/motion vectors for warping textures.

## Features

- Converts P pass to Pref pass via matrix transformation
- Works with Nuke's Axis nodes
- Cross-platform support (Linux, macOS, Windows)
- Optimized C++ implementation for production performance

## Installation

### Pre-compiled Binaries

Pre-compiled plugins are available in the `COMPILED` folder for supported Nuke versions.

1. Download the appropriate `.so` (Linux), `.dylib` (macOS), or `.dll` (Windows) file
2. Copy to your `~/.nuke` directory or any path in your `NUKE_PATH`
3. Restart Nuke

### Building from Source

P2Pref uses CMake with platform-specific configuration files.

#### Requirements

- CMake 3.10+
- C++11 compatible compiler
- Nuke NDK (included with Nuke installation)

#### Linux

```bash
mkdir build && cd build
cp ../CMakeLists_LINUX.txt CMakeLists.txt
cmake . -DNUKE_VERSION=15.0v1 -DCMAKE_INSTALL_PREFIX=~/.nuke
make && make install
```

#### macOS

```bash
mkdir build && cd build
cp ../CMakeLists_MAC.txt CMakeLists.txt
cmake . -DNUKE_VERSION=15.0v1 -DCMAKE_INSTALL_PREFIX=~/.nuke
make && make install
```

#### Windows

```batch
mkdir build && cd build
copy ..\CMakeLists_WIN.txt CMakeLists.txt
cmake . -DNUKE_VERSION=15.0v1 -DCMAKE_INSTALL_PREFIX=%USERPROFILE%\.nuke
cmake --build . --config Release
cmake --install .
```

See `building_step_by_step.txt` for detailed build instructions.

## Usage

1. **Connect your P pass** - Feed a render with world-space position data into P2Pref
2. **Link an Axis/Locator** - Connect a tracked or animated Axis node that represents your projection origin
3. **Output Pref** - The plugin outputs reference-space coordinates

### Basic Node Graph

```
Read (with P pass)
       │
       ▼
    P2Pref ◀──── Axis (tracked to surface)
       │
       ▼
  PRefToMotion
       │
       ▼
    STMap
       │
       ▼
    Merge
```

## Companion Tools

- **[PRefToMotion](https://github.com/petermercell/PRefToMotion)** - Converts Pref pass to UV/motion vectors for texture warping
- **[HOUDINI_NUKE_LOCATOR](https://github.com/petermercell/HOUDINI_NUKE_LOCATOR)** - Export matrix data from Houdini to Nuke Axis nodes

## Compatibility

| Platform | Nuke Version | Status |
|----------|--------------|--------|
| Linux    | 14.1 - 16.x  | ✅     |
| macOS    | 15.0 - 16.x  | ✅     |
| Windows  | 14.1 - 16.x  | ✅     |

## Credits

- **Code base** inspired by [Ivan Busquets'](https://www.linkedin.com/in/ivanbusquets/) C44Matrix
- **Technique** inspired by learnings from [The Next Level](https://www.complairvfx.com/thenextlevel) program by Comp Lair, Pedro Andrade

## License

This project is provided as-is for the VFX community. Big Thanks goes to Comp Lair for allowing me publishing this tool. 

## Author

**Peter Mercell**  
[petermercell.com](https://petermercell.com)

---

*Part of the P2Pref → PRefToMotion texture projection pipeline for Nuke.*
