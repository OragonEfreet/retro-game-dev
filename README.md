# retro-game-dev

A collection of retro video games I want to make, developed using the [Banjo API](https://github.com/OragonEfreet/banjo).  
This project also serves as a playground to drive Banjo’s ongoing development and test its features across different targets.

## Table of contents

- [Build for Web](#build-for-web)
- [Build for Linux](#build-for-linux)
- [Build for Windows](#build-for-windows)
- [Selecting which games to build](#selecting-which-games-to-build)

---

## Build for Web

Web builds are powered by the [Emscripten](https://emscripten.org) WebAssembly framework.

**Requirements:**

- CMake >= 3.21
- Emscripten SDK

Create a folder for building the project, and change to that directory.  
The rest of this guide will assume you are using a `.build_web/` folder directly under the root of this project.

For example:

```sh
mkdir .build_web && cd .build_web
```

First, make sure you installed and activated Emscripten:

```sh
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

cd -
```

Run CMake with Emscripten enabled and build the project:

```sh
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release -DBJ_FEATURE_EMSCRIPTEN=ON
cmake --build . -j
```

When the build completes, `index.html`, the `.js`, and `.wasm` game files will be in your build directory.

### Testing with a local server

**Option 1 – emrun** (recommended for Emscripten testing):

`emrun` is included with the Emscripten SDK and automatically sets the correct MIME types for `.wasm` and `.js` files.

From your build directory, run:

```sh
emrun --port 8000 index.html
```

Add `--no_browser` flag to avoid auto-opening of your default browser.

Then visit:

```
http://localhost:8000/index.html
```

**Option 2 – Python HTTP server** (simple, works without Emscripten active):

```sh
python3 -m http.server 8000
```

Then open [http://localhost:8000/index.html](http://localhost:8000/index.html) in your browser.

---

## Build for Linux

To build native Linux executables with ALSA and X11 enabled (always on for Linux builds):

**Requirements:**

- CMake >= 3.21
- GCC or Clang toolchain
- Development packages for ALSA and X11

From the project root:

```sh
mkdir .build_linux && cd .build_linux
cmake .. -DCMAKE_BUILD_TYPE=Release -DBJ_FEATURE_X11=ON -DBJ_FEATURE_ALSA=ON
cmake --build . -j
```

The compiled game binaries will be in the `.build_linux/` folder.

---

## Build for Windows

To build native Windows executables with Win32 and MME enabled (always on for Windows builds):

**Requirements:**

- CMake >= 3.21
- Visual Studio (MSVC) or MinGW-w64 toolchain

**Using Visual Studio (MSVC):**

```powershell
mkdir .build_win
cd .build_win
cmake .. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release -DBJ_FEATURE_WIN32=ON -DBJ_FEATURE_MME=ON
cmake --build . --config Release
```

**Using MinGW-w64 (from MSYS2 shell):**

```sh
mkdir .build_win && cd .build_win
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DBJ_FEATURE_WIN32=ON -DBJ_FEATURE_MME=ON
cmake --build . -j
```

Binaries will be placed in the `.build_win/` folder.

---

## Selecting which games to build

Each discovered game has a `GAME_<name>` CMake option (default: `ON`).  
To disable building a specific game:

```sh
cmake .. -DGAME_pong=OFF
```

Only enabled games will be built and appear in the `index.html` game list for Web builds.
