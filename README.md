# retro-game-dev

A collection of retro video games I want to make.

## Build for Web

Web builds are powered by the [Emscripten](https://emscripten.org) WebAssembly framework.

**Requirements:**

- CMake >= 3.21
- Emscripten SDK

Create a folder for building the project, and change to that directory.  
The rest of this guide will assume you are using a `.build/` folder directly under the root of this project.

For example:

```sh
mkdir .build && cd .build
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
Serve them with a local HTTP server, for example:

```sh
python3 -m http.server 8000
```

Then open [http://localhost:8000/index.html](http://localhost:8000/index.html) in your browser.

---

## Build for Linux

To build native Linux executables:

**Requirements:**

- CMake >= 3.21
- GCC or Clang toolchain
- Development packages for any optional audio/graphics backends you enable (ALSA, X11, etc.)

From the project root:

```sh
mkdir .build && cd .build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```

The compiled game binaries will be in the `.build/` folder.

---

## Build for Windows

To build native Windows executables:

**Requirements:**

- CMake >= 3.21
- Visual Studio (MSVC) or MinGW-w64 toolchain

**Using Visual Studio (MSVC):**

```powershell
mkdir .build
cd .build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

**Using MinGW-w64 (from MSYS2 shell):**

```sh
mkdir .build && cd .build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```

Binaries will be placed in the `.build/` folder.

---

## Selecting which games to build

Each discovered game has a `GAME_<name>` CMake option (default: `ON`).  
To disable building a specific game:

```sh
cmake .. -DGAME_pong=OFF
```

Only enabled games will be built and appear in the `index.html` game list for Web builds.
