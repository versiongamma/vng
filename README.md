# VNG
 
A x64 game engine developed in C++ using the Vulkan Graphics API

## Frameworks

- Vulkan
- Simple DirectMedia Layer
- Dear ImGui

## Building

### Windows

1. Install the following
 - git
 - Visual Studio (w/ Desktop C++)
 - Vulkan SDK
 
2. Download the source code and dependencies using git:
 > git clone --recursive https://github.com/versiongamma/vng

3. Follow the build instructions for [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) (located in lib/VulkanMemoryAllocator)
4. Open `vng.sln` in Visual Studio
5. Build the solution. The output executable can be found in the `build/` directory 

<img align="right" width="300" height="75" src="https://i.imgur.com/XSfLngf.png"></img>
 