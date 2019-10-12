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

*Resource Ownership*

From such factory methods, instances of such move-only types that represent resources are returned in `cgb::owning_resource` wrappers. These wrappers are, by default, still move-only but offer additional functionality. Their `enable_shared_ownership()` method moves the resource type into a shared pointer internally which has the effect that from that point onwards, the wrapped resource can have multiple owners. Shared ownership, however, should rather be the exception than the rule. Hence, `cgb::owning_resource::enable_shared_ownership` is opt-in. If you enable shared ownership, the represented resource will be moved into heap memory, otherwise it stays on the stack.

There are type aliases for all such resource types wrapped in a `cgb::owning_resource` like the following for the `cgb::image_t` resource type:
```
using image	= owning_resource<image_t>;
```

That means, **ownership** of an image is represented by the type name `cgb::image`, whereas **non-owning usage** of an image is represented by `const cgb::image_t&`, `cgb::image_t&`, `const cgb::image_t*`, or `cgb::image_t*`, where the reference types are far more common. Whenever you encounter a method taking a `cgb::image` parameter, it means that the method needs to take ownership of the image resource. You can either move an image into its parameter, or enable shared ownership on the `owning_resource` wrapper and pass a copy to its parameter. On the contrary, if you encounter a method taking a `const cgb::image_t&` parameter, it means that the method just needs read access to the image. A `owning_resource` wrapper can automatically cast to a const-reference of the type it represents. You don't have to cast manually.

