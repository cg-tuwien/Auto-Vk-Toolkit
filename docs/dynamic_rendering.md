# Dynamic Rendering

**Table of Contents:**     
- [Enabling the Extension](#enabling-the-extension) 
- [Pipeline Creation](#pipeline-creation) 
- [Recording Dynamic Rendering Commands](#recording-dynamic-rendering-commands) 

The standard way of writing renderpasses and pipelines can become overly verbose when the codebase starts to grow. Specifically the need to recreate renderpass objects and framebuffers for each pipeline iteration is of concern. To alleviate this issue Vulkan API provides an extension called dynamic rendering. With this extension, the renderpass and framebuffer construction are no longer constructed statically, typically before the command recording. Instead, their creation is moved inside of the command recording phase, which offers much greater flexibility. Additionally, the pipeline creation needs to be modified to be compatible with the dynamic rendering equation.

## Enabling the Extension

Because dynamic rendering is an extension, it needs to be enabled. This is done by providing the `VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME` into the required device extensions, when creating the composition.

## Pipeline Creation

The creation of the pipeline needs to be sligthly modified to enable dynamic rendering. First, the pipeline needs to be notified, stating that we intend to use it together with dynamic rendering by passing the enum value `avk::cfg::dynamic_rendering::enabled` as a parameter to the pipeline constructor or pipeline creation function `create_graphics_pipeline_for`.

Second, the attachments passed into the pipeline also need to be differently declared: Analogous to how attachments for typical pipelines are created, the API offers three functions for creating dynamic attachments. These are:
- `declare_dynamic(std::tuple<vk::Format, vk::SampleCountFlagBits> aFormatAndSamples, subpass_usages aUsage);`
- `declare_dynamic(vk::Format aFormat, subpass_usages aUsage);`
- `declare_dynamic_for(const image_view_t& aImageView, subpass_usages aUsage);`.

Compared to the typical attachment declaration, these don't contain the load and store operation. It should be noted that dynamic rendering does not support subpasses in any extent. In the context of dynamic attachments, the `aUsage` parameter thus has completely different meaning. Instead of containing multiple subpass usages, `aUsage` must contain only a single subpass usage, which denotes how the attachment will be used in the **dynamic rendering** pass.

Finally, while the option to derive most of the required parameters from a given image view is provided, the attachments created in this way will be compatible with any other image view, that has the same parameters. That is, one can create a pipeline with dynamic attachment created by `declare_dynamic_for()` function, and then provide a completely different image view in the actuall `begin_dynamic_rendering()` call. 

The following shows an example of creating a pipeline for dynamic rendering:
```cpp
auto Pipeline = avk::context().create_graphics_pipeline_for(
	avk::vertex_shader("shader.vert"),
	avk::fragment_shader("shader.frag"),
	avk::cfg::viewport_depth_scissors_config::from_framebuffer(...),
	avk::cfg::dynamic_rendering::enabled,
	avk::attachment::declare_dynamic_for(colorImage, avk::usage::color(0))
);
```

## Recording Dynamic Rendering Commands

Dynamic rendering can be started with the `begin_dynamic_rendering()` command. This command requires two vectors as an input. The first vector must contain all dynamic rendering attachments that will be written to by the renderpass. The second vector contains the image views matching the attachments. It is again important to note, that the attachments do not need to be created from the actual image views used. One can think of these more as a template for the renderpass. That is an attachment created from image view `a` can be used to begin rendering with image view `b` as long as their required parameters are equal.

Additionally to these two vectors, `begin_dynamic_rendering()` has a set of optional parameters. Particularly noteworthy are `aRenderAreaOffset` and `aRenderAreaExtent` used to dynamically declare the bounds of the renderpass. With these, one can clearly start to see the benefits dynamic rendering offers as there is no need to recreate the whole renderpass when the required rendering window changes. Additionally, when no value is provided as `aRenderAreaExtent` the command attempts to automatically deduce the extent from the image views provided. For example, if a renderpass shall cover the entire extent of the image views provided, the auto deduction functionallity will be sufficient (unless the physical extents of the provided image views differ).

To end the renderpass one must call the `end_dynamic_rendering()` function. It is important to end each begun dynamic rendering instance. Dynamic rendering instances are not recursive. Because of this, dynamic rendering instance must be ended before beginning a new one. The code to record a simple dynamic rendering example, which begins dynamic rendering, dispatches a draw call and ends dynamic rendering is provided below:

```cpp
renderCommands.emplace_back(
    avk::command::begin_dynamic_rendering(
		{colorAttachment.value()},
		{colorImageView},
		vkOffset2D{0, 0}));
renderCommands.emplace_back(avk::command::bind_pipeline(pipeline.as_reference()));
renderCommands.emplace_back(avk::command::draw(3u, 1u, 0u, 0u));
renderCommands.emplace_back(avk::command::end_dynamic_rendering());
```
