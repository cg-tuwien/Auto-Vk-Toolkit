# Gears-Vk + Auto-Vk

*Gears-Vk* rendering framework for the Vulkan 1.2 API, implemented in modern C++, using C++17 and C++20 features.     
It aims to hit the sweet spot between programmer-convenience and efficiency while still supporting full Vulkan functionality.
To achieve this goal, this framework uses [*Auto-Vk*](https://github.com/cg-tuwien/Auto-Vk), a convenience and productivity layer atop [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp).

*Gears-Vk* is ready to go. If your system meets the system requirements (see section _Installation_ below), everything is set up to build an run right out of the box. Just open [`visual_studio/gears-vk.sln`](./visual_studio/gears-vk.sln), set one of the example projects as startup project, build and run.

*Note:* At the first run, the Post Build Helper tool is being built. Watch Visual Studio's "Output" tab for status messages and possible instructions.

# Installation

Currently, only Windows is supported as a development platform. The project setup is provided for Visual Studio 2019.

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

_Gears-Vk_ features functionality to automatically update resources (like images or pipelines) after certain _events_ have occured. This can be used to enable the following:
* Shader hot reloading: Update graphics/compute/ray-tracing pipelines after (SPIR-V) shader files have been updated on the file system.
* Adapt to window-resizes: Framebuffer recreation after the swapchain went out of date and was recreated.

**To set up automatic resource-updates,** you have to enable them using the `gvk::updater` class:
```
mPipeline.enable_shared_ownership(); // Make it usable with the updater
mUpdater
  .on(gvk::swapchain_resized_event(gvk::context().main_window()), 
      gvk::shader_files_changed_event(mPipeline))
  .update(mPipeline);
```

In this example, the `gvk::updater mUpdater` is configured so that the `mPipeline` is updated when at least one of two _events_ occurs: either when the swapchain of the main window has been resized, or when one of the `mPipeline`'s shader files have been changed on the file system. 

_Note about swapchain resize:_ The swapchain can only resize if the `window` has been configured to allow being resized (via `window::enable_resizing`).     
_Note about shader files being changed on the file system:_ The loaded shader files are watches for changes, i.e. the SPIR-V versions of shader files in the target directory. The most convenient way to get them updated is to leave the _Post Build Helper_ running in the background. Ensure that its setting "Do not monitor files during app execution" is _not_ enabled, and the _Post Build Helper_ will automatically compile shader files to SPIR-V at runtime if it detects changes to the original shader source files.

The following _events_ are supported:
* `gvk::swapchain_resized_event`: This event occurs when the swapchain's dimensions have changed.
* `gvk::swapchain_changed_event`: This event occurs when swapchain image handles have changed.
* `gvk::files_changed_event`: This event occurs when any of the given files has been modified on the file system.

The following resources can be updated:
* `avk::graphics_pipeline`
* `avk::compute_pipeline`
* `avk::ray_tracing_pipeline`
* `avk::image`
* `avk::image_view`

_Important:_ When you hand over resources that shall be automatically updated to the `gvk::updater`, the `gvk::updater` needs to take ownership of them. Therefore, you'll have to pay the price of enabling shared resource ownership (by the means of `avk::owning_resource::enable_shared_ownership`) for all of these resources.

Usage examples can be found at the following places:
* **hello_world** example at [`hello_world.cpp#L25`](https://github.com/cg-tuwien/Gears-Vk/blob/master/examples/hello_world/source/hello_world.cpp#L25)
* **compute_image_processing** example at [`compute_image_processing.cpp#L13`](https://github.com/cg-tuwien/Gears-Vk/blob/master/examples/compute_image_processing/source/compute_image_processing.cpp#L138)
* **ray_tracing_triangle_meshes** example at [`ray_tracing_triangle_meshes.cpp#L198`](https://github.com/cg-tuwien/Gears-Vk/blob/master/examples/ray_tracing_triangle_meshes/source/ray_tracing_triangle_meshes.cpp#L198).

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

