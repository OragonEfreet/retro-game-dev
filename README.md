# retro-game-dev

A collection of retro video game I want to make

## Build for Web

Web builds are powered by Emscripten WebAssembly framework.

Create a folder, for building the project, change to that directory.
The rest of this guide will assume your are using a _.build/_ folder directly under the root of this project.
For example:

```sh
mkdir .build && cd .build
```

First, make sure you installed and activated Emscripten.
From the root folder of the project, type:

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



