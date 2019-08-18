# cg_base Rendering Framework

cg_base is a modern C++ rendering framework, abstracting several rendering APIs without sacrificing on understandability and offering some nifty features to get you into gears quickly. 

cg_base's core principles are:
* Accelerate low-level graphics development.
* Abstract different rendering APIs in a manner which does not sacrifice significantly on performance, while providing a high level of comfort.
* Don't be a black-box, but instead make the underlying API-calls easy to observe.
* With the above points, be helpful in learning a new graphics API (e.g. learn Vulkan with a OpenGL background).
* Embrace bleeding edge techology and concepts, like C++17 and Real-Time Ray Tracing.
* Be fun and easy to use while helping to produce high quality code.

Support for the following rendering APIs is currently in development:
* Vulkan
* OpenGL 4.5

# Installation

Currently, only Windows is supported as a development platform. The project setup is provided for Visual Studio 2019 only. This might change in the future, however.

Requirements:
* Windows 10 
* Visual Studio 2019 with a Windows 10 SDK installed
* Vulkan SDK 1.1.108.0 (the only SDK version which is supported at the moment)

