# new_vk_engine

## Introduction
Welcome to new_vk_engine, the goal of this project is to create beautiful arts and have fun.

## Features
* Compute Shader
* Texture Generating
* glTF Loading
* Support for HDR
* Volumetric Rendering
* ImGui Integration

## Getting Started
This project uses CMake and Make, to clone and build:

```
1. git clone --recursive https://github.com/qlyjsld/new_vk_engine
2. cd new_vk_engine
3. mkdir build && cd build
4. cmake ..
5. make -j <threads>
6. ./src/vk_engine

require: Vulkan SDK
```
## How to use
See src/main.cpp and shaders/*.comp.

## Demo
![alt text](https://github.com/qlyjsld/new_vk_engine/blob/main/screenshots/cloud.gif)
** *Sunset with phase function, and ambient lighting. highly recommend a HDR monitor for original results.*

## Todo
- [ ] glTF Material
- [ ] Depth Buffer in Compute
- [ ] PBR Lighting

## Contribution
Feel free to send me an email.
