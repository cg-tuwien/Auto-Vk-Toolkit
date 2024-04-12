// Copyright notice:
// 
// This example has been ported from Sasha Willems' "01 Image Processing" example,
// released under MIT license, and provided on GitHub:
// https://github.com/SaschaWillems/Vulkan#ComputeShader Copyright (c) 2016 Sascha Willems
//

#include "imgui.h"
#include "imgui_impl_vulkan.h"

#include "configure_and_compose.hpp"
#include "imgui_manager.hpp"
#include "invokee.hpp"
#include "material_image_helpers.hpp"
#include "sequential_invoker.hpp"
#include "vk_convenience_functions.hpp"

class compute_image_processing_app : public avk::invokee
{
private: // v== Struct definitions and data ==v

	// Define a struct for our vertex input data:
	struct Vertex {
	    glm::vec3 pos;
	    glm::vec2 uv;
	};

	// Define a struct for the uniform buffer which holds the
	// transformation matrices, used in the vertex shader.
	struct MatricesForUbo {
		glm::mat4 projection;
		glm::mat4 model;
	};

	// Vertex data for a quad:
	const std::vector<Vertex> mVertexData = {
		{{  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f }},
		{{ -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f }},
		{{ -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }},
		{{  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }},
	};

	// Indices for the quad:
	const std::vector<uint16_t> mIndices = {
		 0, 1, 2,   2, 3, 0
	};

public: // v== avk::invokee overrides which will be invoked by the framework ==v
	compute_image_processing_app(avk::queue& aQueue)
		: mQueue{ &aQueue }
		, mRotationSpeed{ 1.0f }
	{}
	
	void initialize() override
	{
		// Create and upload vertex data for a quad
		mVertexBuffer = avk::context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(mVertexData)
		);
		avk::context().record_and_submit_with_fence({ mVertexBuffer->fill(mVertexData.data(), 0) }, *mQueue)->wait_until_signalled();

		// Create and upload incides for drawing the quad
		mIndexBuffer = avk::context().create_buffer(
			avk::memory_usage::device, {},
			avk::index_buffer_meta::create_from_data(mIndices)
		);
		avk::context().record_and_submit_with_fence({ mIndexBuffer->fill( mIndices.data(), 0) }, *mQueue)->wait_until_signalled();

		// Create a host-coherent buffer for the matrices
		auto fif = avk::context().main_window()->number_of_frames_in_flight();
		for (decltype(fif) i = 0; i < fif; ++i) {
			mUbo.emplace_back(avk::context().create_buffer(
				avk::memory_usage::host_coherent, {}, // Note: This flag lets the buffer be created in a different memory region than vertex and index buffers above
				avk::uniform_buffer_meta::create_from_data(MatricesForUbo{})
			));
		}

		// Load an image from file, upload it and then create a view and a sampler for it for usage in shaders:
		auto [image, uploadInputImageCommand] = avk::create_image_from_file(
			"assets/lion.png", false, false, true, 4,
			avk::layout::transfer_src, // For now, transfer the image into transfer_dst layout, because we'll need to copy from it:
			avk::memory_usage::device, // The device shall be stored in (fast) device-local memory. For this reason, the function will also return commands that we need to execute later
			avk::image_usage::general_storage_image // Note: We could bind the image as a texture instead of a (readonly) storage image, then we would not need the "storage_image" usage flag 
		);
		// The uploadInputImageCommand will contain a copy operation from a temporary host-coherent buffer into a device-local buffer.
		// We schedule it for execution a bit further down.		

		mInputImageAndSampler = avk::context().create_image_sampler(
			avk::context().create_image_view(image), 
			avk::context().create_sampler(avk::filter_mode::bilinear, avk::border_handling_mode::clamp_to_edge)
		);
		const auto wdth = mInputImageAndSampler->width();
		const auto hght = mInputImageAndSampler->height();
		const auto frmt = mInputImageAndSampler->format();

		// Create an image to write the modified result into, also create view and sampler for that
		mTargetImageAndSampler = avk::context().create_image_sampler(
			avk::context().create_image_view(
				avk::context().create_image( // Create an image and set some properties:
					wdth, hght, frmt, 1 /* one layer */, avk::memory_usage::device, /* in your GPU's device-local memory */
					avk::image_usage::general_storage_image // This flag means (among other usages) that the image can be written to, because it includes the "storage_image" usage flag.
				)
			),
			avk::context().create_sampler(avk::filter_mode::bilinear, avk::border_handling_mode::clamp_to_edge)
		);

		// Execute the uploadInputImageCommand command, wait until that one has completed (by using an automatically created barrier), 
		// then initialize the target image with the contents of the input image:
		avk::context().record_and_submit_with_fence({
			// Copy into the source image:
			std::move(uploadInputImageCommand),

			// Wait until the copy has completed:
			avk::sync::global_memory_barrier(avk::stage::auto_stage >> avk::stage::auto_stage, avk::access::auto_access >> avk::access::auto_access),

			// Transition the target image into a useful image layout:
			avk::sync::image_memory_barrier(mTargetImageAndSampler->get_image(), avk::stage::none >> avk::stage::auto_stage, avk::access::none >> avk::access::auto_access)
				.with_layout_transition(avk::layout::undefined >> avk::layout::general),

			// Copy source to target:
			avk::copy_image_to_another(image.as_reference(), avk::layout::transfer_src, mTargetImageAndSampler->get_image(), avk::layout::general),

			// Finally, transition the source image into general layout for use in compute and graphics pipelines
			avk::sync::image_memory_barrier(image.as_reference(), avk::stage::auto_stage >> avk::stage::none, avk::access::auto_access >> avk::access::none)
				.with_layout_transition(avk::layout::transfer_src >> avk::layout::general),
		}, *mQueue)->wait_until_signalled(); // Finally, wait with a fence until everything has completed.
		
		// Create our rasterization graphics pipeline with the required configuration:
		mGraphicsPipeline = avk::context().create_graphics_pipeline_for(
			avk::from_buffer_binding(0) -> stream_per_vertex(&Vertex::pos) -> to_location(0),
			avk::from_buffer_binding(0) -> stream_per_vertex(&Vertex::uv)  -> to_location(1),
			"shaders/texture.vert",
			"shaders/texture.frag",
			avk::cfg::front_face::define_front_faces_to_be_clockwise(),
			avk::cfg::culling_mode::disabled,
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)).enable_dynamic_viewport(),
			avk::context().create_renderpass({
					avk::attachment::declare(
						avk::format_from_window_color_buffer(avk::context().main_window()), 
						avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::color(0), avk::on_store::store.in_layout(avk::layout::color_attachment_optimal) // Not presentable format yet, because ImGui renders afterwards
					).set_clear_color({0.f, 0.5f, 0.75f, 0.0f}), // Set a different clear color
				}, avk::context().main_window()->renderpass_reference().subpass_dependencies() // Use the same subpass dependencies as main window's renderpass
			),
			// Define bindings:
			avk::descriptor_binding(0, 0, mUbo[0]),	// Just take any UBO, as this is just used to describe the pipeline's layout.
			avk::descriptor_binding<avk::combined_image_sampler_descriptor_info>(0, 1, 1u) 
		);

		// Create 3 compute pipelines:
		mComputePipelines.resize(3);
		mComputePipelines[0] = avk::context().create_compute_pipeline_for(
			"shaders/emboss.comp",
			avk::descriptor_binding(0, 0, mInputImageAndSampler->get_image_view()->as_storage_image(avk::layout::general)),
			avk::descriptor_binding(0, 1, mTargetImageAndSampler->get_image_view()->as_storage_image(avk::layout::general))
		);
		mComputePipelines[1] = avk::context().create_compute_pipeline_for(
			"shaders/edgedetect.comp",
			avk::descriptor_binding(0, 0, mInputImageAndSampler->get_image_view()->as_storage_image(avk::layout::general)),
			avk::descriptor_binding(0, 1, mTargetImageAndSampler->get_image_view()->as_storage_image(avk::layout::general))
		);
		mComputePipelines[2] = avk::context().create_compute_pipeline_for(
			"shaders/sharpen.comp",
			avk::descriptor_binding(0, 0, mInputImageAndSampler->get_image_view()->as_storage_image(avk::layout::general)),
			avk::descriptor_binding(0, 1, mTargetImageAndSampler->get_image_view()->as_storage_image(avk::layout::general))
		);
		
		mUpdater.emplace();
		mUpdater->on(avk::shader_files_changed_event(mComputePipelines[0].as_reference())).update(mComputePipelines[0]);
		mUpdater->on(avk::shader_files_changed_event(mComputePipelines[1].as_reference())).update(mComputePipelines[1]);
		mUpdater->on(avk::shader_files_changed_event(mComputePipelines[2].as_reference())).update(mComputePipelines[2]);
		
		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = avk::context().create_descriptor_cache();
		
		auto* imguiManager = avk::composition_interface::current()->element_by_type<avk::imgui_manager>();
		if(nullptr != imguiManager) {
			imguiManager->add_callback([this, imguiManager](){
		        ImGui::Begin("Info & Settings");
				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
				ImGui::InputFloat("Rotation Speed", &mRotationSpeed, 0.1f, 1.0f);
				
				ImGui::Separator();

				ImTextureID inputTexId = imguiManager->get_or_create_texture_descriptor(mInputImageAndSampler.as_reference(), avk::layout::general);
		        auto inputTexWidth  = static_cast<float>(mInputImageAndSampler->get_image_view()->get_image().create_info().extent.width);
		        auto inputTexHeight = static_cast<float>(mInputImageAndSampler->get_image_view()->get_image().create_info().extent.height);
				ImGui::Text("Input image (%.0fx%.0f):", inputTexWidth, inputTexHeight);
				ImGui::Image(inputTexId, ImVec2(inputTexWidth/6.0f, inputTexHeight/6.0f), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0f,1.0f,1.0f,1.0f), ImVec4(1.0f,1.0f,1.0f,0.5f));

				ImTextureID targetTexId = imguiManager->get_or_create_texture_descriptor(mTargetImageAndSampler.as_reference(), avk::layout::general);
		        auto targetTexWidth  = static_cast<float>(mTargetImageAndSampler->get_image_view()->get_image().create_info().extent.width);
		        auto targetTexHeight = static_cast<float>(mTargetImageAndSampler->get_image_view()->get_image().create_info().extent.height);
				ImGui::Text("Output image (%.0fx%.0f):", targetTexWidth, targetTexHeight);
				ImGui::Image(targetTexId, ImVec2(targetTexWidth/6.0f, targetTexHeight/6.0f), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0f,1.0f,1.0f,1.0f), ImVec4(1.0f,1.0f,1.0f,0.5f));

				ImGui::End();
			});
		}
	}

	void update() override
	{
		assert(!mUpdateToRenderDependency.has_value());

		// Handle some input:
		if (avk::input().key_pressed(avk::key_code::num0)) {
			// [0] => Copy the input image to the target image and use a semaphore to sync the next draw call
			auto semaphore = avk::context().record_and_submit_with_semaphore({
				// Copy source to target:
				avk::copy_image_to_another(mInputImageAndSampler->get_image(), avk::layout::general, mTargetImageAndSampler->get_image(), avk::layout::general),
			}, *mQueue, avk::stage::auto_stage);

			// We'll wait for it in render():
			mUpdateToRenderDependency = std::move(semaphore);
		}
		else if (avk::input().key_down(avk::key_code::num1) || avk::input().key_pressed(avk::key_code::num2) || avk::input().key_pressed(avk::key_code::num3)) {
			// [1], [2], or [3] => Use a compute shader to modify the image
			
			size_t computeIndex = 0;
			if (avk::input().key_pressed(avk::key_code::num2)) { computeIndex = 1; }
			if (avk::input().key_pressed(avk::key_code::num3)) { computeIndex = 2; }

			auto& commandPool = avk::context().get_command_pool_for_single_use_command_buffers(*mQueue);
			auto cmdbfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
			cmdbfr->begin_recording();

			auto semaphore = avk::context().record_and_submit_with_semaphore({
				// Bind the compute pipeline:
				avk::command::bind_pipeline(mComputePipelines[computeIndex].as_reference()),

				// Bind all the resources:
				avk::command::bind_descriptors(mComputePipelines[computeIndex]->layout(), mDescriptorCache->get_or_create_descriptor_sets({
					avk::descriptor_binding(0, 0, mInputImageAndSampler->get_image_view()->as_storage_image(avk::layout::general)),
					avk::descriptor_binding(0, 1, mTargetImageAndSampler->get_image_view()->as_storage_image(avk::layout::general))
				})),

				// Make a dispatch call:
				avk::command::dispatch(mInputImageAndSampler->width() / 16, mInputImageAndSampler->height() / 16, 1),
			}, *mQueue, avk::stage::auto_stage);

			// We'll wait for it in render():
			mUpdateToRenderDependency = std::move(semaphore);
		}

		if (avk::input().key_pressed(avk::key_code::escape) || avk::context().main_window()->should_be_closed()) {
			// Stop the current composition:
			avk::composition_interface::current()->stop();
		}
	}

	void render() override
	{
		// Update the UBO's data:
		auto* mainWnd = avk::context().main_window();
		const auto w = mainWnd->swap_chain_extent().width;
		const auto halfW = w * 0.5f;
		const auto h = mainWnd->swap_chain_extent().height;
		
		MatricesForUbo uboVS{};
		uboVS.projection = glm::perspective(glm::radians(60.0f), w * 0.5f / h, 0.1f, 256.0f);
		uboVS.model = glm::translate(glm::mat4{1.0f}, glm::vec3(0.0f, 0.0f, -3.0));
		uboVS.model = uboVS.model * glm::rotate(glm::mat4{1.0f}, glm::radians(avk::time().time_since_start() * mRotationSpeed * 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		// Update the buffer:
		const auto ifi = mainWnd->current_in_flight_index();
		auto emptyCommands = mUbo[ifi]->fill(&uboVS, 0); // We are updating the buffer in host-coherent memory.
		// ^ Because of its memory region (host-coherent), we can be sure that the returned commands are empty. Hence, we do not need to execute them.
		
		// Get a command pool to allocate command buffers from:
		auto& commandPool = avk::context().get_command_pool_for_single_use_command_buffers(*mQueue);

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();
		
		// Create a command buffer and render into the *current* swap chain image:
		auto cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		// Prepare two viewports:
		auto vpLeft = vk::Viewport{ 0.0f, 0.0f, halfW, static_cast<float>(h) };
		auto vpRight = vk::Viewport{ halfW, 0.0f, halfW, static_cast<float>(h) };

		auto submission = avk::context().record({
				// Begin and end one renderpass:
				avk::command::render_pass(mGraphicsPipeline->renderpass_reference(), avk::context().main_window()->current_backbuffer_reference(), {

					// Draw left viewport:
					avk::command::custom_commands([&,this](avk::command_buffer_t& cb) { // If there is no avk::command::... struct for a particular command, we can always use avk::command::custom_commands
						cb.handle().setViewport(0, 1, &vpLeft);
					}),

					// Bind a pipeline and perform an indexed draw call:
					avk::command::bind_pipeline(mGraphicsPipeline.as_reference()),
					avk::command::bind_descriptors(mGraphicsPipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
						avk::descriptor_binding(0, 0, mUbo[ifi]),
						avk::descriptor_binding(0, 1, mInputImageAndSampler->as_combined_image_sampler(avk::layout::general))
					})),
					avk::command::draw_indexed(mIndexBuffer.as_reference(), mVertexBuffer.as_reference()),

					// Draw right viewport:
					avk::command::custom_commands([&,this](avk::command_buffer_t& cb) { // If there is no avk::command::... struct for a particular command, we can always use avk::command::custom_commands
						cb.handle().setViewport(0, 1, &vpRight);
					}),

					// Bind a pipeline and perform an indexed draw call:
					avk::command::bind_pipeline(mGraphicsPipeline.as_reference()),
					avk::command::bind_descriptors(mGraphicsPipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
						avk::descriptor_binding(0, 0, mUbo[ifi]),
						avk::descriptor_binding(0, 1, mTargetImageAndSampler->as_combined_image_sampler(avk::layout::general))
					})),
					avk::command::draw_indexed(mIndexBuffer.as_reference(), mVertexBuffer.as_reference())
				})
				
			})
			.into_command_buffer(cmdBfr)
			.then_submit_to(*mQueue)
			// Do not start to render before the image has become available:
			.waiting_for(imageAvailableSemaphore >> avk::stage::color_attachment_output)
			.store_for_now();

		if (mUpdateToRenderDependency.has_value()) {
			// If there are some (pending) updates submitted from update(), establish a dependency to them:
			submission.waiting_for(mUpdateToRenderDependency.value() >> avk::stage::fragment_shader); // Images are read in the fragment_shader stage

			cmdBfr->handle_lifetime_of(std::move(mUpdateToRenderDependency.value()));
			mUpdateToRenderDependency.reset();
		}

		submission.submit();

		// Use a convenience function of avk::window to take care of the command buffer's lifetime:
		// It will get deleted in the future after #concurrent-frames have passed by.
		avk::context().main_window()->handle_lifetime(std::move(cmdBfr));
	}

private: // v== Member variables ==v

	avk::queue* mQueue;
	avk::buffer mVertexBuffer;
	avk::buffer mIndexBuffer;
	std::vector<avk::buffer> mUbo;
	avk::image_sampler mInputImageAndSampler;
	avk::image_sampler mTargetImageAndSampler;
	avk::descriptor_cache mDescriptorCache;

	avk::graphics_pipeline mGraphicsPipeline;

	std::vector<avk::compute_pipeline> mComputePipelines;

	std::optional<avk::semaphore> mUpdateToRenderDependency;

	float mRotationSpeed;
};

int main() // <== Starting point ==
{
	int result = EXIT_FAILURE;
	try {
		// Create a window and open it
		auto* mainWnd = avk::context().create_window("Compute Image Effects Example");
		mainWnd->set_resolution({ 1600, 800 });
		mainWnd->set_presentaton_mode(avk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = avk::context().create_queue(vk::QueueFlagBits::eCompute, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->set_queue_family_ownership(singleQueue.family_index());
		mainWnd->set_present_queue(singleQueue);

		// Create an instance of our main avk::element which contains all the functionality:
		auto app = compute_image_processing_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = avk::imgui_manager(singleQueue);
		
		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			avk::application_name("Auto-Vk-Toolkit Example: Compute Image Effects Exampl"),
			[](avk::validation_layers& config) {
				//config.enable_feature(vk::ValidationFeatureEnableEXT::eSynchronizationValidation);
			},
			// Pass windows:
			mainWnd,
			// Pass invokees:
			app, ui
		);

		// Create an invoker object, which defines the way how invokees/elements are invoked
		// (In this case, just sequentially in their execution order):
		avk::sequential_invoker invoker;

		// With everything configured, let us start our render loop:
		composition.start_render_loop(
			// Callback in the case of update:
			[&invoker](const std::vector<avk::invokee*>& aToBeInvoked) {
				// Call all the update() callbacks:
				invoker.invoke_updates(aToBeInvoked);
			},
			// Callback in the case of render:
				[&invoker](const std::vector<avk::invokee*>& aToBeInvoked) {
				// Sync (wait for fences and so) per window BEFORE executing render callbacks
				avk::context().execute_for_each_window([](avk::window* wnd) {
					wnd->sync_before_render();
					});

				// Call all the render() callbacks:
				invoker.invoke_renders(aToBeInvoked);

				// Render per window:
				avk::context().execute_for_each_window([](avk::window* wnd) {
					wnd->render_frame();
					});
			}
			); // This is a blocking call, which loops until avk::current_composition()->stop(); has been called (see update())

		result = EXIT_SUCCESS;
	}
	catch (avk::logic_error&) {}
	catch (avk::runtime_error&) {}
	return result;
}
