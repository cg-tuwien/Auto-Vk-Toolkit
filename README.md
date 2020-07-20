# Exekutor + Auto-Vk

*Exekutor* is a modern C++17-based rendering framework for the Vulkan 1.2 API.      
It aims to hit the sweet spot between programmer-convenience and efficiency while still supporting full Vulkan functionality.    
To achieve this goal, this framework uses [*Auto-Vk*](https://github.com/cg-tuwien/Auto-Vk), a convenience and productivity layer atop [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp).

# Installation

Currently, only Windows is supported as a development platform. The project setup is provided for Visual Studio 2019 only. This might change in the future, however.

Requirements:
* Windows 10 
* Visual Studio 2019 with a Windows 10 SDK installed
* Vulkan SDK 1.2.141.0 or newer

Detailed information about project setup and resource management with Visual Studio are given in [`visual_studio/README.md`](./visual_studio/README.md)

## FAQs, Known Issues, Troubleshooting

**Q: I have troubles with asset management in Visual Studio. Any advice?**        
_A: Check out [Known Issues and Troubleshooting w.r.t. Resource Handling](https://github.com/cg-tuwien/cg_base/blob/master/visual_studio/README.md#known-issues-and-troubleshooting-wrt-asset-handling), which offers guidelines for the following cases:_      
[Build errors when adding assets](https://github.com/cg-tuwien/cg_base/blob/master/visual_studio/README.md#build-errors-when-adding-assets)         
[Asset is not deployed because it is not saved in the Visual Studio's filters-file](https://github.com/cg-tuwien/cg_base/blob/master/visual_studio/README.md#asset-is-not-deployed-because-it-is-not-saved-in-the-visual-studios-filters-file)     

**Q: I have troubles with the CGB Post Build Helper. What to do?**        
_A: Check out [Known Issues and Troubleshooting w.r.t. CGB Post Build Helper](https://github.com/cg-tuwien/cg_base/tree/master/visual_studio#known-issues-and-troubleshooting-wrt-cgb-post-build-helper), which offers guidelines for the following cases:_        
[Application could not start at first try (maybe due to missing DLLs)](https://github.com/cg-tuwien/cg_base/tree/master/visual_studio#application-could-not-start-at-first-try-maybe-due-to-missing-dlls)        
[Error message about denied access to DLL files (DLLs are not re-deployed)](https://github.com/cg-tuwien/cg_base/tree/master/visual_studio#error-message-about-denied-access-to-dll-files-dlls-are-not-re-deployed)      
[Too few resources are being deployed](https://github.com/cg-tuwien/cg_base/tree/master/visual_studio#too-few-resources-are-being-deployed)      
[Slow performance when showing lists within CGB Post Build Helper](https://github.com/cg-tuwien/cg_base/tree/master/visual_studio#slow-performance-when-showing-lists-within-cgb-post-build-helper)      

**Q: The application takes a long time to load assets like 3D models and images. Can it be accelerated?**     
_A: If you are referring to the Debug-build, you can configure CGB Post Build Helper so that it deploys Release-DLLs of some external dependencies which should accelerate asset loading a lot._     
To do that, please open CGB Post Build Helper's [Settings dialogue](https://github.com/cg-tuwien/cg_base/tree/master/visual_studio#settings) and enable the option "Always deploy Release DLLs".

