# Automatic Resource-Updates

_Gears-Vk_ features functionality to automatically update resources (like images or pipelines), or invoke callbacks after certain _events_ have occured. Currently the following event types are supported:
* `gvk::swapchain_resized_event`: This event occurs when the swapchain's dimensions have changed.
* `gvk::swapchain_changed_event`: This event occurs when the swapchain is recreated for any reason.
* `gvk::swapchain_format_changed_event`: This event occurs when the image format of the swapchain has changed. Currently, this can be triggered when invoking `window::request_srgb_framebuffer` for the given window.
* `gvk::swapchain_additional_attachments_changed_event`: This event occurs when window's additional attachments change. The set of additional attachments may change when invoking `window::set_additional_back_buffer_attachments`.
* `gvk::concurrent_frames_count_changed_event`: This event occurs when the number of concurrent frames has been changed by the user. The number of concurrent frames is modified by invoking `window::set_number_of_concurrent_frames`.
* `gvk::destroying_graphics_pipeline_event`: This event occurs when an outdated graphics pipeline is destroyed.
* `gvk::destroying_compute_pipeline_event`: This event occurs when an outdated compute pipeline is destroyed.
* `gvk::destroying_ray_tracing_pipeline_event`: This event occurs when an outdated ray tracing pipeline is destroyed.
* `gvk::destroying_image_event`: This event occurs when an outdated image is destroyed.
* `gvk::destroying_image_view_event`: This event occurs when an outdated image view is destroyed.
* `gvk::files_changed_event`: This event occurs when any of the given files have been modified on the file system.


## How to use

The event handling and the automatic updater mechanisms are available via `gvk::updater` class. The access to an updater is restricted to objects of type `gvk::invokee`. This means that each `gvk::invokee` object possesses its own `gvk::updater` instance, functioning in isolation from other updater instances. An invokee's updater is unset by default and can be activated by the following call:
```
mUpdater.emplace();
```
The general schema for using an updater is the following:

```
mUpdater.on(event... events).[update(...)|invoke(...)][.then_on(event... events).[update(...)|invoke(...)]]*
```

* `events`: A comma separated list of event objects. The events are in a logical _or_ relationship, which means that the bound callbacks are invoked if _any_ of the events occur.
* `on(event... events)`: The list of events that trigger the subsequently specified actions defined by `update` or `invoke` calls.
* `update(updatee... updatees)`: Comma separated list of updatees which are updated after _any_ of the previously defined events has occured. The following resources can be updated by passing them to this method:
  * `avk::graphics_pipeline`
  * `avk::compute_pipeline`
  * `avk::ray_tracing_pipeline`
  * `avk::image`
  * `avk::image_view`
* `invoke(handler... handlers)`: invokes the callbacks specified by the comma separated list once the bound event occurs. The following function signatures are allowed:
  * `std::function<void()>`: For general use.
  * `std::function<void(const avk::graphics_pipeline&)>`: In combination with `gvk::destroying_graphics_pipeline_event`.
  * `std::function<void(const avk::compute_pipeline&)>`: In combination with `gvk::destroying_compute_pipeline_event`.
  * `std::function<void(const avk::ray_tracing_pipeline&)>`: In combination with `gvk::destroying_ray_tracing_pipeline_event`.
  * `std::function<void(const avk::image&)>`: In combination with `gvk::destroying_image_event`.
  * `std::function<void(const avk::image_view&)>`: In combination with `gvk::destroying_image_view_event`.
* `then_on(event... events)`: bind the following up `update` or `invoke` call to a list of events. However, unlike `on(event... events)`, the given events will be evaluated _after_ the updates induced by previous `update` or `invoke` calls have finished execution.

It is important to note that the order in which the events are evaluated is not necessarily determined by the order of the events in each separate list.

### Examples

```
mPipeline.enable_shared_ownership(); // Make it usable with the updater
mUpdater->on(gvk::swapchain_resized_event(gvk::context().main_window()), 
      gvk::shader_files_changed_event(mPipeline))
  .update(mPipeline);
```

In the above example, the `gvk::updater mUpdater` is configured so that the `mPipeline` is updated when at least one of two _events_ occurs: either when the swapchain of the main window has been resized, or when one of the `mPipeline`'s shader files have been changed on the file system. 

```
mUpdater->on(
      gvk::swapchain_format_changed_event(gvk::context().main_window()),
      gvk::swapchain_additional_attachments_changed_event(gvk::context().main_window())
   ).invoke([this]() {
      // update render pass here
   }).then_on(
      gvk::swapchain_changed_event(gvk::context().main_window()),
      gvk::shader_files_changed_event(mPipeline)
   ).update(mPipeline);
```

In the above example, the renderpass belonging to `mPipeline` requires recreation only if the events `swapchain_format_changed_event` or `swapchain_format_changed_event` arise. Then it is evaluated whether _any type_ of swapchain recreation has occured and if so, subsequently the pipeline is updated. It is important to note here: The evaluation of events given to `then_on()` is only temporaly restricted by the previous list: this evaluation will occur in any case.

## Common Use-Cases
* Swapchain recreation can lead to changes to image properties. This may cause any pipeline, renderpass, framebuffer or image view that directly works with swapchain to end in an invalid state. The updater mechanism is meant to streamline the recreation process of those objects, as well as to allow any user-defined behaviour to execute in cases where such events occur.
* Shader hot reloading: Update graphics/compute/ray-tracing pipelines after (SPIR-V) shader files have been updated on the file system.
* Adapt to changes in the number of concurrent frames, by setting up possibly required synchronization mechanisms.

## Notes on Swapchain recreation

* Modifying the following attributes may trigger a swapchain recreation:
  * window size,
  * presentation mode,
  * number of presentation images,
  * number of concurrent frames,
  * backbuffer image format,
  * out of data, or suboptimal swapchain detection by Vulkan.
* The swapchain can only resize if the `window` has been configured to allow being resized (via `window::enable_resizing`).   

## General Notes
* The updater mechanism is invoked just before the `render()` call of the associated `gvk::invokee`.
* When resources are handed over to the updater mechanism to be updated automatically, the `gvk::updater` needs to take ownership of them. Therefore, you'll have to pay the price of enabling shared resource ownership (by the means of `avk::owning_resource::enable_shared_ownership`) for all of these resources.
* _Note about shader files being changed on the file system:_ The loaded shader files are watched for changes, i.e. the SPIR-V versions of shader files in the target directory. The most convenient way to get them updated is to leave the _Post Build Helper_ running in the background. Ensure that its setting "Do not monitor files during app execution" is _not_ enabled. The _Post Build Helper_ will automatically compile shader files to SPIR-V at runtime if it detects changes to the original shader source files.
* A `avk::graphics_pipeline` object will re-use its previously assigned renderpass without modification after recreation.

## Examples

Usage examples can be found at the following places:
* **hello_world** example at [`hello_world.cpp#L25`](https://github.com/cg-tuwien/Gears-Vk/blob/master/examples/hello_world/source/hello_world.cpp#L25)
* **model_loader** example at [`model_loader.cpp#L178`](https://github.com/cg-tuwien/Gears-Vk/blob/master/examples/model_loader/source/model_loader.cpp#L178)
* **compute_image_processing** example at [`compute_image_processing.cpp#L141`](https://github.com/cg-tuwien/Gears-Vk/blob/master/examples/compute_image_processing/source/compute_image_processing.cpp#L141)
* **ray_tracing_triangle_meshes** example at [`ray_tracing_triangle_meshes.cpp#L213`](https://github.com/cg-tuwien/Gears-Vk/blob/master/examples/ray_tracing_triangle_meshes/source/ray_tracing_triangle_meshes.cpp#L213).
