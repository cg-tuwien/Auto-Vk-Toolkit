# Gears-Vk + Auto-Vk

*Gears-Vk* is a modern C++17-based rendering framework for the Vulkan 1.2 API.      
It aims to hit the sweet spot between programmer-convenience and efficiency while still supporting full Vulkan functionality.
To achieve this goal, this framework uses [*Auto-Vk*](https://github.com/cg-tuwien/Auto-Vk), a convenience and productivity layer atop [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp).

*Gears-Vk* is ready to go. If your system meets the system requirements (see section _Installation_ below), everything is set up to build an run right out of the box. Just open [`visual_studio/gears-vk.sln`](./visual_studio/gears-vk.sln), set one of the example projects as startup project, build and run.

*Note:* At the first run, the Post Build Helper tool is being built. Watch Visual Studio's "Output" tab for status messages and possible instructions.

# Installation

Currently, only Windows is supported as a development platform. The project setup is provided for Visual Studio 2019 only.

Requirements:
* Windows 10 
* Visual Studio 2019 with a Windows 10 SDK installed
* Vulkan SDK 1.2.141.0 or newer

Detailed information about project setup and resource management with Visual Studio are given in [`visual_studio/README.md`](./visual_studio/README.md)

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
* Shader Hot Reloading: Update graphics/compute/ray-tracing pipelines after (SPIR-V) shader files have been updated on the file system.
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

**Q: I have troubles with asset management in Visual Studio. Any advice?**        
_A: Check out [Known Issues and Troubleshooting w.r.t. Resource Handling](https://github.com/cg-tuwien/cg_base/blob/master/visual_studio/README.md#known-issues-and-troubleshooting-wrt-asset-handling), which offers guidelines for the following cases:_      
[Build errors when adding assets](https://github.com/cg-tuwien/cg_base/blob/master/visual_studio/README.md#build-errors-when-adding-assets)         
[Asset is not deployed because it is not saved in the Visual Studio's filters-file](https://github.com/cg-tuwien/cg_base/blob/master/visual_studio/README.md#asset-is-not-deployed-because-it-is-not-saved-in-the-visual-studios-filters-file)     

**Q: I have troubles with the Post Build Helper. What to do?**        
_A: Check out [Known Issues and Troubleshooting w.r.t. CGB Post Build Helper](https://github.com/cg-tuwien/cg_base/tree/master/visual_studio#known-issues-and-troubleshooting-wrt-cgb-post-build-helper), which offers guidelines for the following cases:_        
[Application could not start at first try (maybe due to missing DLLs)](https://github.com/cg-tuwien/cg_base/tree/master/visual_studio#application-could-not-start-at-first-try-maybe-due-to-missing-dlls)        
[Error message about denied access to DLL files (DLLs are not re-deployed)](https://github.com/cg-tuwien/cg_base/tree/master/visual_studio#error-message-about-denied-access-to-dll-files-dlls-are-not-re-deployed)      
[Too few resources are being deployed](https://github.com/cg-tuwien/cg_base/tree/master/visual_studio#too-few-resources-are-being-deployed)      
[Slow performance when showing lists within CGB Post Build Helper](https://github.com/cg-tuwien/cg_base/tree/master/visual_studio#slow-performance-when-showing-lists-within-cgb-post-build-helper)      

**Q: The application takes a long time to load assets like 3D models and images. Can it be accelerated?**     
_A: If you are referring to the Debug-build, you can configure CGB Post Build Helper so that it deploys Release-DLLs of some external dependencies which should accelerate asset loading a lot._     
To do that, please open CGB Post Build Helper's [Settings dialogue](https://github.com/cg-tuwien/cg_base/tree/master/visual_studio#settings) and enable the option "Always deploy Release DLLs".

