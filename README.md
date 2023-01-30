# Wicked Engine - Demo
## STATUS: Pre-Production (Come and join our [Discord](https://discord.gg/CFjRYmE))

## Support Devs
|Developer|Support|
|---|---|
|Turánszki János|[![Patreon](https://img.shields.io/badge/donate-patreon-blue?style=for-the-badge&logo=patreon&color=E35B57&logoColor=FFFFFF&labelColor=232323)](https://patreon.com/wickedengine)|
|Megumumpkin|TBA|
|MolassesLover|[![Ko-Fi](https://img.shields.io/badge/donate-kofi-blue?style=for-the-badge&logo=ko-fi&color=E35B57&logoColor=FFFFFF&labelColor=232323)](https://ko-fi.com/molasses) [![Patreon](https://img.shields.io/badge/donate-patreon-blue?style=for-the-badge&logo=patreon&color=E35B57&logoColor=FFFFFF&labelColor=232323)](https://www.patreon.com/molasseslover)|


A fully-fledged game made using [Wicked Engine](https://github.com/turanszkij/WickedEngine) 
in order to battle-test features, mature the engine, and expand the community. 

</br>

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

</br>

## Building

### Step 1 - Git Clone this repository

```sh
➜ git clone --recursive https://github.com/megumumpkin/WickedEngine-Demo.git
```

### Step 2 - Build WickedEngine Library (not automatically built!)

Linux
```sh
➜ cd Library/WickedEngine
➜ mkdir build
➜ cd build
➜ cmake .. -DCMAKE_BUILD_TYPE=Release
➜ make WickedEngine_Linux -j$(nproc)
➜ cd ../../../ && ls
```
Windows
```sh
➜ cd Library\WickedEngine\
➜ mkdir build
➜ cd build
➜ cmake ..
➜ cmake --build . --target WickedEngine_Windows --config Release
```

### Step 3 - Build The Game and Dev Tool

Linux
```sh
➜ mkdir -p 'Data/Shader'
➜ mkdir -p 'Data/Content'
➜ cmake -B Build -DWickedEngine_DIR=Library/WickedEngine/build/cmake . 
➜ cmake --build Build -j$(nproc)
```

Windows
```sh
➜ cmake -B Build -DWickedEngine_DIR=Library/WickedEngine/Build/cmake . 
➜ cmake --build Build --config Release
➜ mkdir -p "Build\Release\Data\Content"
➜ xcopy /e /i "Library\WickedEngine\WickedEngine\shaders" "Build\Release\Data\Shader"
➜ xcopy /i /e "Library\WickedEngine\WickedEngine\dxcompiler.dll" "Build\Release\dxcompiler.dll"
```

### Step 4 - Launch Game or Game.exe (depends on your platform of choice)

You can launch by terminal/cmd or just click the executable.
The first launch will:
- Create an .ini file for configuration
- Compiles all WickedEngine's and this game's shaders

</br>

## Developer's CLI

There's another executable named Dev / Dev.exe in the built folder or the downloaded package. This will be used as a tool to manage game assets, like importing and previews.

Launch the program through terminal/cmd, and try the command below

Linux
```
➜ ./Dev -h
```

Windows
```
➜  Dev.exe -h
```

## You Like to Design Levels and Create Assets?

Get yourself this [**Blender Plugin**](https://github.com/megumumpkin/Redline-Studio) and start creating game data within the `Data/Content` folder!
Make sure that the `Data` folder resides at the same place as the `Dev` / `Dev.exe` executable!

Download the program to import assets to engine on the Releases tab of this project page, especially the one with the name Dev Release 202X.XX