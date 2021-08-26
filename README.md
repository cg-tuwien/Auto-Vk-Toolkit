# Gears-Vk + Auto-Vk

*Gears-Vk* rendering framework for the Vulkan 1.2 API, implemented in modern C++, using C++17 and C++20 features.     
It aims to hit the sweet spot between programmer-convenience and efficiency while still supporting full Vulkan functionality.
To achieve this goal, this framework uses [*Auto-Vk*](https://github.com/cg-tuwien/Auto-Vk), a convenience and productivity layer atop [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp).

*Gears-Vk* is ready to go. If your system meets the system requirements (see section _Installation_ below), everything is set up to build an run right out of the box. Just open [`visual_studio/gears-vk.sln`](./visual_studio/gears-vk.sln), set one of the example projects as startup project, build and run.

*Note:* At the first run, the Post Build Helper tool is being built. Watch Visual Studio's "Output" tab for status messages and possible instructions.

# Installation

## Visual Studio 2019
A preconfigured project setup is provided for Visual Studio 2019 on Windows.

Requirements:
* Windows 10 
* Visual Studio 2019 with a Windows 10 SDK installed (For detailed information about project setup and resource management please refer to [`visual_studio/README.md`](./visual_studio/README.md).)
* A [Vulkan 1.2 SDK from LunarG](https://vulkan.lunarg.com/sdk/home), optimally Vulkan SDK 1.2.141.0 or newer

Setup and build instructions:
* Clone this repository
* Execute a `git submodule update --init` to pull the [_Auto-Vk_](https://github.com/cg-tuwien/Auto-Vk) framework which is added as a submodule under `auto_vk`
* Open the Visual Studio solution file [`visual_studio/gears-vk.sln`](./visual_studio/), and build the solution
* During building, you'll recognize messages from the _Post Build Helper_ tool in Visual Studio's `Output`-tab, some popup messages, and an icon in the system tray. Please have a look at section [Resource Mangement and the Post Build Helper](#resource-mangement-and-the-post-build-helper) for additional information.
* Several example applications are available in the solution file. Set one of them as startup project, and run.

Set up your own project:
* To add _Gears-Vk_ to one of your custom repositories, you might want to add it as a GIT submodule. You could execute `git submodule add https://github.com/cg-tuwien/Gears-Vk.git gears_vk` to add _Gears-Vk_ as submodule in directory `gears_vk`.
* Execute `git submodule update --init --recursive` in order to pull both, _Gears-Vk_ and the [_Auto-Vk_](https://github.com/cg-tuwien/Auto-Vk) framework.
* The steps described under section [Creating a New Project](#creating-a-new-project) might be helpful for setting up a custom Visual Studio project that links agains _Gears-Vk_.

## CMake
_Gears-Vk_ also supports building with CMake on Linux (*GCC* or *Clang*) and Windows (*MSVC*).

There are currently no pre-built binaries of *Gears-Vk*'s dependencies for Linux included in the repository, so they are built alongside *Gears-Vk* the first time you build it.
However, if you have an *Assimp* build installed on your system (so that it can be found via `find_package(assimp)`) it will be used instead (e.g. on Ubuntu 20.04 you can `sudo apt-get install libassimp-dev` to get *Assimp* version 5.0.1).
On Windows pre-built binaries of dependencies are included and used when building *Gears-Vk*.

### CMake Options
You can configure the build process by setting the following options:

| Name | Description | Default |
| ---- | ----------- | ------- |
| `gvk_LibraryType` | The type of library gvk should be built as. Must be `INTERFACE`, `SHARED` or `STATIC` | `STATIC` |
| `gvk_ForceAssimpBuild` | Forces a local build of *Assimp* even if it is installed on the system. (Linux only) | `OFF` |
| `gvk_StaticDependencies` | Sets if dependencies (*Assimp* & *GLFW*) should be built as static instead of shared libraries. (Linux only) | `OFF` |
| `gvk_ReleaseDLLsOnly` | Sets if release DLLS (*Assimp* & *STB*) should be used for examples, even for debug builds. (Windows only) | `ON` |
| `gvk_CreateDependencySymlinks` | Sets if dependencies of examples, i.e. DLLs (Windows only) & assets, should be copied or if symbolic links should be created. | `ON` |
| `gvk_BuildExamples` | Build all examples for *Gears-Vk*. | `OFF` |
| `gvk_BuildHelloWorld` | Build example: hello_world. | `OFF` |
| `gvk_BuildFramebuffer` | Build example: framebuffer. | `OFF` |
| `gvk_BuildComputeImageProcessing` | Build example: compute_image_processing. | `OFF` |
| `gvk_BuildMultiInvokeeRendering` | Build example: multi_invokee_rendering. | `OFF` |
| `gvk_BuildModelLoader` | Build example: model_loader. | `OFF` |
| `gvk_BuildOrcaLoader` | Build example: orca_loader. | `OFF` |
| `gvk_BuildRayQueryInRayTracingShaders` | Build example: ray_query_in_ray_tracing_shaders. | `OFF` |
| `gvk_BuildRayTracingWithShadowsAndAO` | Build example: ray_tracing_with_shadows_and_ao. | `OFF` |
| `gvk_BuildRayTracingCustomIntersection` | Build example: ray_tracing_custom_intersection. | `OFF` |
| `gvk_BuildTextureCubemap` | Build example: texture_cubemap. | `OFF` |
| `gvk_BuildVertexBuffers` | Build example: vertex_buffers. | `OFF` |

#### Windows CMake build settings
There are three different build settings for the examples (i.e. `gvk_BuildExamples` is `ON`) on Windows allowing you to select examples as `Startup Item` in Visual Studio:
* `x64-Debug`: Produces debug builds, while using release DLLs and prefers symbolic links for dependencies.
* `x64-Release`: Produces release builds and prefers symbolic links for dependencies.
* `x64-Publish`: Produces release builds and copies dependencies.

### Post Build Commands (Cmake)
For compiling shaders and copying (or creating symbolic links to) dependencies to the same location as an executable target, you can use the provided CMake function `add_post_build_commands`.
It has the following signature:

```Cmake
add_post_build_commands(
        target              # the executable target (e.g. hello_world)
        glslDirectory       # the absolute path of the directory containing GLSL shaders used by the target
        spvDirectory        # the absolute path of the directory where compiled SPIR-V shaders should be written to
        assetsDirectory     # the absolute path of the directory where assets should be copied to (or where symbolic links should be created) - can be a generator expression
        assets              # a list containing absolute paths of assets which should be copied to ${assetsDirectory} - can be files or directories
        symlinks)           # a boolean setting if symbolic links of assets (and DLLs on Windows) should be created instead of copying dependencies
```

All shader files found in the given `glslDirectory` are added to a custom target (named `${target}_shaders`) which is added to the given `target` as a dependency.
Shaders belonging to this custom target will be compiled before the dependent `target` is build.
Whenever a shader file changes they will be recompiled automatically.
Note, however, that new shader files are only added to the custom target at configure time, so if you're working with an IDE you must make sure to trigger CMake to reconfigure its build files whenever you add a new shader.
E.g. on CLion or Visual Studio you can temporarily add a `message()` call as a work-around.

Note that creating symbolic links might require the user running CMake to have special privileges. E.g. on Windows the user needs the `Create symbolic links` privilege.
If the user doesn't have the required privileges `add_post_build_commands` falls back to copying the dependencies.

### Creating a New Project (CMake)
To create a new *Gears-Vk* project using CMake you can use the [Gears-Vk-Starter template](https://github.com/JolifantoBambla/Gears-Vk-Starter).

# Creating a New Project

For Visual Studio projects, there is a convenience tool under [`visual_studio/tools/executables/`](visual_studio/tools/executables) that can help to quickly set up a new project by copying an existing one (e.g. one of the example applications): `create_new_project.exe`

Use it like follows to create a copy of an existing project:
* Open `create_new_project.exe` and either select one of the example applications or enter the path to the project to be copied manually.
* Enter the target location, the target project name, and hit the _[Create it!]_-button.
* The project is copied to the target folder and all relative paths are adapted to the new location if the target folder is on the same drive as the source project. (If it is not, absolute paths are set.)
* Asset references and shader references are retained and their paths are adapted.       
  _Attention:_ Make sure to remove the existing references if you are going to modify the referenced assets/shaders! You'll have to create copies of these files manually and add references to the copies instead. If you fail to do so, you'll end up modifying the stock assets or the examples' shader files.
* Precompiled headers are disabled in the newly created project copy. If you'd like to use this feature, you'll have to manually enable it in Visual Studio's project settings.
* Manually add a reference to the _Gears-Vk_ library project [`gears-vk.vxcproj`](visual_studio/gears_vk) to your Visual Studio solution and ensure that the newly created project copy references it.
* All source and include file references are removed from the newly created project copy. You'll have to add at least a `.cpp` file containing a `main()` function.
* Add `#include <gvk.hpp>` to use _Gears-Vk_.
* After these steps, you should be able to successfully link against _Gears-Vk_ build your newly created project.

A good strategy is to add _Gears-Vk_ as a **git submodule** to your repository and use `create_new_project.exe` and the steps above to create a properly configured project in a directory outside of the submodule. Make sure to frequently update the submodule by pulling from _Gears-Vk_'s `master` branch to get the latest updates.

# Resource Mangement and the Post Build Helper

_Gears-Vk_'s Visual Studio projects are configured so that Visual Studio itself can be elegantly used for resource management. That means, required assets (3D models, images, [ORCA](https://developer.nvidia.com/orca) scene files) and shader files can just be added to Visual Studio's filters in the "Solution Explorer" view and a smart _Post Build Helper_ tool ensures that those resources are deployed to the application's target directory.

In short:
* Add required 3D models, images, and ORCA scenes to the `assets` filter, and
* add required shader files to the `shaders` filter
directly in Visual Studio. Then build the application, wait for the _Post Build Helper_ to deploy these resources to the target directory, and run your application!

This can look like follows, where the filters `assets` and `shaders` have special meaning, as hinted above:    
<img src="visual_studio/docs/images/orca_loader_filters.png" width="292"/>

A more detailed explanation and further instructions are given in [`visual_studio/README.md`](visual_studio/README.md).

You will notice _Post Build Helper_ activity through its tray icon: <img src="visual_studio/docs/images/PBH_tray.png" />. The tool will remain active after deployment has finished for two main reasons:
* It allows to investigate logs from previous build events, and also change settings.
* It continues to monitor resource files which is especially important to enable **shader hot reloading**.

For more information about the _Post Build Helper_, please refer to the [Post Build Helper](visual_studio/README.md#post-build-helper) section, and for more information about shader hot reloading, please refer to the [Automatic Resource-Updates](#automatic-resource-updates) section below.

# What's the difference between Gears-Vk and Auto-Vk?

*Auto-Vk* is a platform-agnostic convenience and productivity layer atop Vulkan-Hpp. 

*Gears-Vk* establishes the missing link to the operating system -- in this case Windows 10 -- and adds further functionality:
* Rendering environment configuration, such as enabling Vulkan extensions (e.g. if `VK_KHR_raytracing` shall be used, it selects an appropriate physical device and enables required flags and extensions)
* Window management (through GLFW)
* Game-loop/render-loop handling with convenient to use callback methods via the `gvk::invokee` interface (such as `initialize()`, `update()`, `render()`, where the former is called only once and the latter two are invoked each frame)
* User input handling
* A ready to use base class for object hierarchies: `gvk::transform`
* A ready to use user-controllable camera class `gvk::quake_camera` (which is derived from both, `gvk::transform` and `gvk::invokee`)
* Resource loading support for:
  * Images
  * 3D Models
  * Scenes in the ORCA format, see: [ORCA: Open Research Content Archive](https://developer.nvidia.com/orca)
* Material loading and conversion into a GPU-suitable format (`gvk::material` and `gvk::material_gpu_data`)
* Lightsource loading and conversion into a GPU-suitable format (`gvk::lightsource` and `gvk::lightsource_gpu_data`)
* Resource handling via Visual Studio's filters, i.e. just drag and drop assets and shaders that you'd like to use directly into Visual Studio's filter hierarchy and get them deployed to the target directory.
* A powerful Post Build Helper tool which is invoked as a custom build step.
  * It deploys assets and shaders to the target directory
  * Shaders are compiled into SPIR-V
  * If shader files contain errors, popup messages are created displaying the error, and providing a `[->VS]` button to navigate to the line that contains the error _within_ Visual Studio.
  * By default, "Debug" and "Release" build configurations symlink resources to save space, but "Publish" build configurations deploy all required files into the target directory so that a built program can easily be transfered to another PC. No more tedious resource gathering is required in such situations since that is all handled by the Post Build Helper.

# Automatic Resource-Updates

See: [Automatic Resource-Updates](./docs/updater.md)

# FAQs, Known Issues, Troubleshooting

**Q: Can Gears-Vk be used on Linux?**           
_A:_ Not yet. [Auto-Vk](https://github.com/cg-tuwien/Auto-Vk), however, can.

**Q: Can Gears-Vk be used without the _Post Build Helper_?**      
_A:_ Yes. The _Post Build Helper_ is a convenience tool that handles resource deployment, asset dependencies, and also file updates (useful for shader hot reloading, depending on the project structure). If you're not using it, you'll have to manage deployment of resources, and compilation of shader files into SPIR-V manually.

**Q: I have troubles with asset management in Visual Studio. Any advice?**        
_A:_ Check out [Known Issues and Troubleshooting w.r.t. Asset Handling](./visual_studio#known-issues-and-troubleshooting-wrt-asset-handling), which offers guidelines for the following cases:      
* [Build errors when adding assets](./visual_studio#build-errors-when-adding-assets)
* [Asset is not deployed because it is not saved in the Visual Studio's filters-file](./visual_studio#asset-is-not-deployed-because-it-is-not-saved-in-the-visual-studios-filters-file)  

**Q: More resources have been deployed than I have added to Visual Studio's filters. What's going on?**      
_A:_ Some assets reference other assets internally. For example, 3D models often reference images or material files (in case of `.obj` models). These "dependent assets" are also deployed to the target directory by the _Post Build Helper_. Please see [Deployment of Dependent Assets](./visual_studio#deployment-of-dependent-assets) for more details.

**Q: What are the differences between _Debug_, _Release_, and _Publish_ build configurations?**      
_A:_ In terms of compilation settings, _Release_ and _Publish_ configurations are the same. They link against _Release_ builds of libraries. _Debug_ configuration has classical debug settings configured for the Visual Studio projects and links against _Debug_ builds of libraries. There is, however, a difference between _Publish_ builds and non-_Publish_ builds w.r.t. the deployment of resources. Please see [Symbolic Links/Copies depending on Build Configuration](./visual_studio#symbolic-linkscopies-depending-on-build-configuration) for more details.

**Q: I have troubles with the _Post Build Helper_. What to do?**        
_A:_ Check out [Post Build Helper](./visual_studio#post-build-helper), which offers guidelines for the following cases:  
* [Build is stuck at "Going to invoke[...]MSBuild.exe" step, displayed in Visual Studio's Output tab](./visual_studio#build-is-stuck-at-going-to-invokemsbuildexe-step-displayed-in-visual-studios-output-tab)
* [Post Build Helper can't be built automatically/via MSBuild.exe](./visual_studio#post-build-helper-cant-be-built-automaticallyvia-msbuildexe)
* [Too few resources are being deployed](./visual_studio#too-few-resources-are-being-deployed)
* [Application could not start at first try (maybe due to missing assets or DLLs)](./visual_studio#application-could-not-start-at-first-try-maybe-due-to-missing-assets-or-dlls)
* [Error message about denied access to DLL files (DLLs are not re-deployed)](./visual_studio#error-message-about-denied-access-to-dll-files-dlls-are-not-re-deployed)
* [Slow performance when showing lists within the Post Build Helper](./visual_studio#slow-performance-when-showing-lists-within-the-post-build-helper)

**Q: The application takes a long time to load assets like 3D models and images. Can it be accelerated?**     
_A:_ If you are referring to _Debug_ builds, you can configure _Post Build Helper_ so that it deploys _Release_ DLLs of some external dependencies even for _Debug_ builds. They should accelerate asset loading a lot. To enable deployment of _Release_ DLLs, please open _Post Build Helper_'s [settings](./visual_studio#post-build-helper-settings) and enable the option "Always deploy Release DLLs".

