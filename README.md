# Wicked Engine - Demo
[![Ko-Fi](https://img.shields.io/badge/donate-kofi-blue?style=for-the-badge&logo=ko-fi&color=E35B57&logoColor=FFFFFF&labelColor=232323)](https://ko-fi.com/molasses)
[![Patreon](https://img.shields.io/badge/donate-patreon-blue?style=for-the-badge&logo=patreon&color=E35B57&logoColor=FFFFFF&labelColor=232323)](https://www.patreon.com/molasseslover)

A fully-fledged game made using [Wicked Engine](https://github.com/turanszkij/WickedEngine) 
in order to battle-test features, mature the engine, and expand the community. 

This repository contains submodules in the [`Library/`](Library/) directory, you 
might want to clone those!

```sh 
➜ git clone https://github.com/MolassesLover/WickedEngine-Demo.git --recursive
➜ cd WickedEngine-Demo
➜ git submodule update Library/WickedEngine
```

 <html>
  <div class="container">
      <img src="https://user-images.githubusercontent.com/60114762/162796909-dc754428-c4d1-47f4-9c80-82d3e3b35d71.png">
  </div>
</html>

> Artwork by [@Megumumpkin](https://github.com/megumumpkin)

## Dependencies
In order to build this project you will need a few pieces of software on your
system. Depending on your operating system and its distribution, some of these 
dependencies might already be met. In any case, dependencies are fairly minimal.

Here is a full list of dependencies:

- [Wicked Engine](https://github.com/turanszkij/WickedEngine)
- [CMake](https://cmake.org/)
- [Vulkan](https://www.vulkan.org/)
- [SDL2](https://www.libsdl.org/download-2.0.php)
- [DXC](https://github.com/Microsoft/DirectXShaderCompiler)

## Building

First things first, make sure you have a fully-built version of 
[Wicked Engine](https://github.com/turanszkij/WickedEngine) somewhere on
your system. It's also worth mentioning that it should be compiled using CMake.

This project also uses CMake, so do make sure that dependency is met.

### Manually
Replace `/path/to/wicked/build_folder/` with your Wicked Engine `build/` directory.
```sh
➜ cmake -B Build -DWickedEngine_DIR=/path/to/wicked/build_folder/cmake .
➜ cmake --build Build -j$(nproc)
```

### Automatically
You can run the [`Build.sh`](Source/Building/Build.sh) script in order to build
both [Wicked Engine](https://github.com/turanszkij/WickedEngine)  and this demo project. 
Just make sure you have the [Wicked Engine submodule](Library/WickedEngine/) in the 
root directory of this repository. Otherwise, you can clone it with this command:

```sh 
➜ git submodule update --init Library/WickedEngine
```
