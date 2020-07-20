# *Exekutor* Vulkan Rendering Framework featuring *Auto-Vk*

*Exekutor* is a modern C++17-based rendering framework for the Vulkan 1.2 API. It aims to hit the sweet spot between programmer-convenience and efficiency while still supporting full Vulkan functionality.

# Installation

Currently, only Windows is supported as a development platform. The project setup is provided for Visual Studio 2019 only. This might change in the future, however.

Requirements:
* Windows 10 
* Visual Studio 2019 with a Windows 10 SDK installed
* Vulkan SDK 1.2.141.0 or newer

Detailed information about project setup and resource management with Visual Studio are given in [`visual_studio/README.md`](./visual_studio/README.md)

# Documentation

This section explains some of the most important concepts of cg_base. Overall, documentation is lagging behind a bit, but the following sub-sections should cover the most important fundamental specifics in terms of framework architecture.

## Move-only Types

Many of cg_base's types are move-only types. That means, they can not be copy constructed or copy assigned. This approach is taken for all types which represent a resource uniquely, like, for example, a handle to a GPU resource. You are able to see whether or not a type is move-only by looking at its declaration. The type `cgb::image_t` is such a move-only type and its definition shows that the copy constructor and copy assignment operator are deleted:

``` 
class image_t
{
public:
	image_t() = default;
	image_t(const image_t&) = delete;
	image_t(image_t&&) = default;
	image_t& operator=(const image_t&) = delete;
	image_t& operator=(image_t&&) = default;
	~image_t() = default;
  // ...
};
``` 

## Type Naming, `::create` Methods, and `owning_resource` Wrapper

Such move-only types which represent a resource, are named after the resource they represent plus a `_t` postfix. Example: the type, representing a GPU image resource is named `image_t` and (as every framework type) resides in the `cgb` namespace, i.e. the full type name is `cgb::image_t`.

For most of the types representing resources, but also for other types in the framework, `static` factory methods exist which can and should be used to create these types. In many cases, these functions are called `create` or their names start with "create". For the `cgb::image_t` type, some examples of such factory methods are the following:

```
class image_t
{
public:
  // ...
  static owning_resource<image_t> create(uint32_t pWidth, uint32_t pHeight, image_format pFormat, bool pUseMipMaps = false, int pNumLayers = 1, memory_usage pMemoryUsage = memory_usage::device, image_usage pImageUsage = image_usage::versatile_image, context_specific_function<void(image_t&)> pAlterConfigBeforeCreation = {});
  
  static owning_resource<image_t> create_depth(uint32_t pWidth, uint32_t pHeight, std::optional<image_format> pFormat = std::nullopt, bool pUseMipMaps = false, int pNumLayers = 1,  memory_usage pMemoryUsage = memory_usage::device, image_usage pImageUsage = image_usage::read_only_depth_stencil_attachment, context_specific_function<void(image_t&)> pAlterConfigBeforeCreation = {});
  // ...
};
```

They can be used like follows:
```
auto depthImage = cgb::image_t::create_depth(1920, 1080, cgb::image_format::default_depth_format());
```

**Resource Ownership**

From such factory methods, instances of such move-only types that represent resources are returned in `cgb::owning_resource` wrappers. These wrappers are, by default, still move-only but offer additional functionality. Their `enable_shared_ownership()` method moves the resource type into a shared pointer internally which has the effect that from that point onwards, the wrapped resource can have multiple owners. Shared ownership, however, should rather be the exception than the rule. Hence, `cgb::owning_resource::enable_shared_ownership` is opt-in. If you enable shared ownership, the represented resource will be moved into heap memory, otherwise it stays on the stack.

There are type aliases for all such resource types wrapped in a `cgb::owning_resource` like the following for the `cgb::image_t` resource type:
```
using image	= owning_resource<image_t>;
```

That means, *ownership* of an image is represented by the type name `cgb::image`, whereas *non-owning usage* of an image is represented by `const cgb::image_t&`, `cgb::image_t&`, `const cgb::image_t*`, or `cgb::image_t*`, where the reference types are far more common. Whenever you encounter a method taking a `cgb::image` parameter, it means that the method needs to take ownership of the image resource. You can either move an image into its parameter, or enable shared ownership on the `owning_resource` wrapper and pass a copy to its parameter. On the contrary, if you encounter a method taking a `const cgb::image_t&` parameter, it means that the method just needs read access to the image. A `owning_resource` wrapper can automatically cast to a const-reference of the type it represents. You don't have to cast manually.

## Synchronization by Semaphores

The description below is no longer true. Synchronization has undergone major refactoring, now also offering full support for barriers, while keeping semaphores as an option. See `cgb::sync` for more details for now. The description here will be updated.

~Often, you'll encounter scenarios where an operation on the GPU needs to wait until a previous operation has finished on the GPU. Whenever synchronization can not be handled internally, cg_base handles such synchronization dependencies via semaphores.~

~Whenever a semaphore CAN occur inside some method, the method will offer a **semaphore handler** which has a signature like follows: `std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler`. It means that the semaphore handler must take care of the semaphore's ownership and handle it somehow. Such semaphore handlers are generally *optional*. If you do not pass a semaphore handler, the semaphore will be handled internally. However, that could result in bad performance, because internally, cg_base will issue a "wait idle" call, meaning that it will wait until a device queue or the device has completed all pending work before resuming. You will see warnings on the console in such cases.~

~In order to handle a semaphore, you will want to pass it on as a "wait semaphore" to a command which requires the work represented by the original semaphore to be completed. Such semaphore dependencies can be provided to methods via `std::vector<semaphore> _WaitSemaphores` parameters.~

~In many cases, you'll just require the rendering of the current frame to wait on the completion of an operation. For such cases, you can forward the semaphore to a window. In the following example, a semaphore is created for the creating and filling of a vertex buffer, and is set as an extra semaphore dependency to the rendering of the current frame via `cgb::window`'s `set_extra_semaphore_dependency` method:~

```
auto vertexPositionsBuffer = cgb::create_and_fill(
  cgb::vertex_buffer_meta::create_from_data(newElement.mPositions),
  cgb::memory_usage::device,
  vertexPositions.data(),
  // Handle the semaphore, if one gets created (which will happen 
  // since we've requested to upload the buffer to the device)
  [] (auto _Semaphore) {  
  	cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
  }
);
```
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

