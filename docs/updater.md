# Automatic Resource-Updates

_Auto-Vk-Toolkit_ features functionality to automatically update resources (like images or pipelines), or invoke callbacks after certain _events_ have occured. Currently the following event types are supported:
* `avk::swapchain_resized_event`: This event occurs when the swapchain's dimensions have changed.
* `avk::swapchain_changed_event`: This event occurs when the swapchain is recreated for any reason.
* `avk::swapchain_format_changed_event`: This event occurs when the image format of the swapchain has changed. Currently, this can be triggered when invoking `window::request_srgb_framebuffer` for the given window.
* `avk::swapchain_additional_attachments_changed_event`: This event occurs when a window's additional attachments change. The set of additional attachments may change when invoking `window::set_additional_back_buffer_attachments`.
* `avk::concurrent_frames_count_changed_event`: This event occurs when the number of concurrent frames has been changed by the user. The number of concurrent frames is modified by invoking `window::set_number_of_concurrent_frames`.
* `avk::destroying_graphics_pipeline_event`: This event occurs when an outdated graphics pipeline is destroyed.
* `avk::destroying_compute_pipeline_event`: This event occurs when an outdated compute pipeline is destroyed.
* `avk::destroying_ray_tracing_pipeline_event`: This event occurs when an outdated ray tracing pipeline is destroyed.
* `avk::destroying_image_event`: This event occurs when an outdated image is destroyed.
* `avk::destroying_image_view_event`: This event occurs when an outdated image view is destroyed.
* `avk::files_changed_event`: This event occurs when any of the associated files have been modified on the file system.


## How to use

The event handling and the automatic updater mechanisms are available via instances of the `avk::updater` class. The access to an updater is restricted to objects of type `avk::invokee`. This means that each `avk::invokee` object possesses its own `avk::updater` instance, functioning in isolation from other updater instances. An invokee's updater is unset by default and can be activated by the following call:
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
* `invoke(handler... handlers)`: Comma-separated list of handlers which are invoked after _any_ of the previously defined events has occurred. The following function signatures are allowed:
  * `std::function<void()>`: For general use.
  * `std::function<void(const avk::graphics_pipeline&)>`: In combination with `avk::destroying_graphics_pipeline_event`.
  * `std::function<void(const avk::compute_pipeline&)>`: In combination with `avk::destroying_compute_pipeline_event`.
  * `std::function<void(const avk::ray_tracing_pipeline&)>`: In combination with `avk::destroying_ray_tracing_pipeline_event`.
  * `std::function<void(const avk::image&)>`: In combination with `avk::destroying_image_event`.
  * `std::function<void(const avk::image_view&)>`: In combination with `avk::destroying_image_view_event`.
* `then_on(event... events)`: Establish a chain of updates, where the updaters linked to `then_on` are invoked after the updaters that precede this method call. Unlike `on(event... events)`, the given events will be evaluated _after_ the updates induced by previous `update` or `invoke` calls have finished execution.

It is important to note that the order in which the events are evaluated is not necessarily determined by the order of the events in each separate list.

### Code Examples

#### Updating the graphics pipeline  
```
mUpdater->on(avk::swapchain_resized_event(avk::context().main_window()), 
        avk::shader_files_changed_event(mPipeline.as_reference())
    ).update(mPipeline);
```

In the above example, the `avk::updater mUpdater` is configured so that the `mPipeline` is updated when at least one of two _events_ occurs: either when the swapchain of the main window has been resized, or when one of the `mPipeline`'s shader files have been changed on the file system. 

#### Establishing an ordered update chain

```
mUpdater->on(
      avk::swapchain_format_changed_event(avk::context().main_window()),
      avk::swapchain_additional_attachments_changed_event(avk::context().main_window())
   ).invoke([this]() {
      // update render pass here
   }).then_on(
      avk::swapchain_changed_event(avk::context().main_window()),
      avk::shader_files_changed_event(mPipeline.as_reference())
   ).update(mPipeline);
```

In the above example, the renderpass belonging to `mPipeline` requires recreation only if the events `swapchain_format_changed_event` or `swapchain_format_changed_event` arise. Then it is evaluated whether a swapchain recreation has occured and if so, the pipeline is updated subsequently. It is important to note that the evaluation of events given to `then_on()` is only temporally restricted by the previous list of events. The second evaluation (represented by `then_on`) will occur in any case.

## Common Use-Cases
* Swapchain recreation may lead to changes in other, dependent images to be necessary. This may cause any pipeline, renderpass, framebuffer or image view that directly works with swapchain to end in an invalid state. The updater mechanism is meant to streamline the recreation process of those objects, as well as to allow any user-defined behaviour to execute in cases where such events occur.
* Shader hot reloading: Update graphics/compute/ray tracing pipelines after (SPIR-V) shader files have been updated on the file system.
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
* The updater mechanism is invoked just before the `render()` call of the associated `avk::invokee`.
* When resources are handed over to the updater mechanism to be updated automatically, the `avk::updater` needs to take ownership of them.
* _Note about shader files being changed on the file system:_ The loaded shader files are watched for changes, i.e. the SPIR-V versions of shader files in the target directory. The most convenient way to get them updated is to leave the _Post Build Helper_ running in the background. Ensure that its setting "Do not monitor files during app execution" is _not_ enabled. The _Post Build Helper_ will automatically compile shader files to SPIR-V at runtime if it detects changes to the original shader source files.
* A `avk::graphics_pipeline` object will re-use its previously assigned renderpass without modification after recreation.

## Example Applications

Usage examples can be found at the following places:
* **hello_world** example at [`hello_world.cpp`](../examples/hello_world/source/hello_world.cpp#L28)
* **model_loader** example at [`model_loader.cpp`](../examples/model_loader/source/model_loader.cpp#L179)
* **compute_image_processing** example at [`compute_image_processing.cpp`](../examples/compute_image_processing/source/compute_image_processing.cpp#L162)
* **ray_tracing_with_shadows_and_ao** example at [`ray_tracing_with_shadows_and_ao.cpp`](../examples/ray_tracing_with_shadows_and_ao/source/ray_tracing_with_shadows_and_ao.cpp#L104) (ensure to set the following compile flag as follows: `#define ENABLE_SHADER_HOT_RELOADING_FOR_RAY_TRACING_PIPELINE 1`!).
