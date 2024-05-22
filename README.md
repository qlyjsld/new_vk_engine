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
2. mkdir build && cd build
3. cmake ..
4. make -j <threads>
5. ./src/vk_engine
```

**Warning** ** may not build or run on your machine. **

## How to use
See src/main.cpp and shaders/*.comp to begin.

## Demo
![alt text](https://github.com/qlyjsld/new_vk_engine/blob/main/screenshots/cloud.png)
** *Tone mapped volmetric rendering, highly recommend a HDR monitor for original results.*

![alt text](https://github.com/qlyjsld/new_vk_engine/blob/main/screenshots/sunset.png)
** *Sunset with phase function, and ambient lighting.*

![alt text](https://github.com/qlyjsld/new_vk_engine/blob/main/screenshots/spherical_atmosphere.png)
** *Spherical atmosphere.*

## Todo
- [ ] Dynamic Descriptors Pool
- [ ] Dynamic Pipelines
- [ ] Texture cache
- [ ] glTF Material
- [ ] Depth Buffer in Compute
- [ ] PBR Lighting

## Contribution
Make your pull requests. I am open to any kind of collaboration, email me if you want.
