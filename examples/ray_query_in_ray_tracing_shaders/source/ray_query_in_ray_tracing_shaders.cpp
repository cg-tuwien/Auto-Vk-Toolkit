#include "auto_vk_toolkit.hpp"
#include "imgui.h"

#include "configure_and_compose.hpp"
#include "imgui_manager.hpp"
#include "invokee.hpp"
#include "triangle_mesh_geometry_manager.hpp"
#include "quake_camera.hpp"
#include "sequential_invoker.hpp"
#include "vk_convenience_functions.hpp"

// Set this compiler switch to 1 to enable hot reloading of
// the ray tracing pipeline's shaders. Set to 0 to disable it.
#define ENABLE_SHADER_HOT_RELOADING_FOR_RAY_TRACING_PIPELINE 1

// Set this compiler switch to 1 to make the window resizable
// and have the pipeline adapt to it. Set to 0 ti disable it.
#define ENABLE_RESIZABLE_WINDOW 1

// Data to be pushed to the GPU along with a specific draw call:
struct push_const_data {
	glm::vec4  mAmbientLight;
	glm::vec4  mLightDir;
	glm::mat4  mCameraTransform;
	float mCameraHalfFovAngle;
	float _padding;
	vk::Bool32  mEnableShadows;
	float mShadowsFactor;
	glm::vec4  mShadowsColor;
	vk::Bool32  mEnableAmbientOcclusion;
	float mAmbientOcclusionMinDist;
	float mAmbientOcclusionMaxDist;
	float mAmbientOcclusionFactor;
	glm::vec4  mAmbientOcclusionColor;
};

// Main invokee of this application:
class ray_query_in_ray_tracing_shaders_invokee : public avk::invokee
{
public: // v== avk::invokee overrides which will be invoked by the framework ==v
	ray_query_in_ray_tracing_shaders_invokee(avk::queue& aQueue)
		: mQueue{ &aQueue }
	{}

	void initialize() override
	{
		// Create a descriptor cache that helps us to conveniently create descriptor sets,
		// which describe where shaders can find resources like buffers or images:
		mDescriptorCache = avk::context().create_descriptor_cache();

		// Set the direction towards the light:
		mLightDir = { 0.8f, 1.0f, 0.0f };

		// Get a pointer to the main window:		
		auto* mainWnd = avk::context().main_window();

		// Create an offscreen image to ray-trace into. It is accessed via an image view:
		const auto wdth = avk::context().main_window()->resolution().x;
		const auto hght = avk::context().main_window()->resolution().y;
		const auto frmt = avk::format_from_window_color_buffer(mainWnd);
		auto offscreenImage = avk::context().create_image(wdth, hght, frmt, 1, avk::memory_usage::device, avk::image_usage::general_storage_image);
		avk::context().record_and_submit_with_fence({
			avk::sync::image_memory_barrier(offscreenImage.as_reference(), avk::stage::none >> avk::stage::none, avk::access::none >> avk::access::none)
				.with_layout_transition(avk::layout::undefined >> avk::layout::general)
		}, *mQueue)->wait_until_signalled();
		mOffscreenImageView = avk::context().create_image_view(offscreenImage);

		// The triangle_mesh_geometry_manager has lower execution order. Therefore, we can
		// assume that it already contains the data that we require:
		auto* triMeshGeomMgr = avk::current_composition()->element_by_type<triangle_mesh_geometry_manager>();
		assert(nullptr != triMeshGeomMgr);

		// Initialize the TLAS (but don't build it yet)
		mTlas = avk::context().create_top_level_acceleration_structure(
			triMeshGeomMgr->max_number_of_geometry_instances(), // <-- Specify how many geometry instances there are expected to be
			true               // <-- Allow updates since we want to have the opportunity to enable/disable some of them via the UI
		);

		// Create our ray tracing pipeline with the required configuration:
		mPipeline = avk::context().create_ray_tracing_pipeline_for(
			// Specify all the shaders which participate in rendering in a shader binding table (the order matters):
			avk::define_shader_table(
				// Our shader binding table only consists of three shaders.
				// We do not need other shaders since we are handling all the things that
				// require recursive rays via ray queries in this example. For an alternative
				// approach which uses more shaders in the shader binding table and does not
				// use ray queries, have a look at the ray_query_in_ray_tracing_shaders_invokee example.
				avk::ray_generation_shader("shaders/ray_gen_shader.rgen"),
				avk::triangles_hit_group::create_with_rchit_only("shaders/closest_hit_shader.rchit"),
				avk::miss_shader("shaders/miss_shader.rmiss")
			),
			// We won't need the maximum recursion depth, but why not:
			avk::context().get_max_ray_tracing_recursion_depth(),
			// Define push constants and descriptor bindings:
			avk::push_constant_binding_data{ avk::shader_type::ray_generation | avk::shader_type::closest_hit, 0, sizeof(push_const_data) },
			avk::descriptor_binding(0, 0, triMeshGeomMgr->combined_image_sampler_descriptor_infos()),
			avk::descriptor_binding(0, 1, triMeshGeomMgr->material_buffer()),
			avk::descriptor_binding(0, 2, avk::as_uniform_texel_buffer_views(triMeshGeomMgr->index_buffer_views())),
			avk::descriptor_binding(0, 3, avk::as_uniform_texel_buffer_views(triMeshGeomMgr->tex_coords_buffer_views())),
			avk::descriptor_binding(0, 4, avk::as_uniform_texel_buffer_views(triMeshGeomMgr->normals_buffer_views())),
			avk::descriptor_binding(1, 0, mOffscreenImageView->as_storage_image(avk::layout::general)), // Bind the offscreen image to render into as storage image
			avk::descriptor_binding(2, 0, mTlas)                                                        // Bind the TLAS, s.t. we can trace rays against it
		);

		// Print the structure of our shader binding table, also displaying the offsets:
		mPipeline->print_shader_binding_table_groups();

#if ENABLE_SHADER_HOT_RELOADING_FOR_RAY_TRACING_PIPELINE || ENABLE_RESIZABLE_WINDOW
		// Create an updater:
		mUpdater.emplace();

#if ENABLE_SHADER_HOT_RELOADING_FOR_RAY_TRACING_PIPELINE
		mUpdater->on(avk::shader_files_changed_event(mPipeline.as_reference()))
			.update(mPipeline);
#endif

#if ENABLE_RESIZABLE_WINDOW
		mOffscreenImageView.enable_shared_ownership(); // The updater needs to hold a reference to it, so we need to enable shared ownership.
		mUpdater->on(avk::swapchain_resized_event(avk::context().main_window()))
			.update(mOffscreenImageView, mPipeline)
			.then_on(avk::destroying_image_view_event()) // Make sure that our descriptor cache stays cleaned up:
			.invoke([this](const avk::image_view& aImageViewToBeDestroyed) {
			auto numRemoved = mDescriptorCache->remove_sets_with_handle(aImageViewToBeDestroyed->handle());
		});
#endif
#endif

		// Add the camera to the composition (and let it handle the updates)
		mQuakeCam.set_translation({ 0.0f, 10.0f, 45.0f });
		mQuakeCam.set_perspective_projection(glm::radians(60.0f), avk::context().main_window()->aspect_ratio(), 0.5f, 100.0f);
		avk::current_composition()->add_element(mQuakeCam);
		
		// Add an "ImGui Manager" which handles the UI:
		auto imguiManager = avk::current_composition()->element_by_type<avk::imgui_manager>();
		if (nullptr != imguiManager) {
			imguiManager->add_callback([
				this,
				timestampPeriod = std::invoke([]() {
					// get timestamp period from physical device, could be different for other GPUs
					auto props = avk::context().physical_device().getProperties();
					return static_cast<double>(props.limits.timestampPeriod);
				}),
				lastFrameDurationMs = 0.0,
				lastTraceRaysDurationMs = 0.0
			]() mutable {
				ImGui::Begin("Info & Settings");
				ImGui::SetWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
				ImGui::SetWindowSize(ImVec2(400.0f, 410.0f), ImGuiCond_FirstUseEver);

				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
				ImGui::Separator();
				ImGui::TextColored(ImVec4(.5f, .3f, .4f, 1.f), "Timestamp Period: %.3f ns", timestampPeriod);
				lastFrameDurationMs = glm::mix(lastFrameDurationMs, mLastFrameDuration * 1e-6 * timestampPeriod, 0.05);
				lastTraceRaysDurationMs = glm::mix(lastTraceRaysDurationMs, mLastTraceRaysDuration * 1e-6 * timestampPeriod, 0.05);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "Frame time (timer queries): %.3lf ms", lastFrameDurationMs);
				ImGui::TextColored(ImVec4(.8f, .1f, .6f, 1.f), "TraceRays took (t.queries): %.3lf ms", lastTraceRaysDurationMs);
				ImGui::Separator();
				ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), "[F1]: Toggle input-mode");
				ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), " (UI vs. scene navigation)");

				// Let the user change the ambient color:
				ImGui::DragFloat3("Ambient Light", glm::value_ptr(mAmbientLight), 0.001f, 0.0f, 1.0f);

				// Let the user change the light's direction, which also influences shadows:
				ImGui::DragFloat3("Light Direction", glm::value_ptr(mLightDir), 0.005f, -1.0f, 1.0f);
				mLightDir = glm::normalize(mLightDir);

				// Let the user change the field of view, and evaluate in the ray generation shader:
				ImGui::DragFloat("Field of View", &mFieldOfViewForRayTracing, 1, 10.0f, 160.0f);

				ImGui::Separator();
				// Let the user change shadow parameters:
				ImGui::Checkbox("Enable Shadows", &mEnableShadows);
				if (mEnableShadows) {
					ImGui::SliderFloat("Shadows Intensity", &mShadowsFactor, 0.0f, 1.0f);
					ImGui::ColorEdit3("Shadows Color", glm::value_ptr(mShadowsColor));
				}

				ImGui::Separator();
				// Let the user change ambient occlusion parameters:
				ImGui::Checkbox("Enable Ambient Occlusion", &mEnableAmbientOcclusion);
				if (mEnableAmbientOcclusion) {
					ImGui::DragFloat("AO Rays Min. Length", &mAmbientOcclusionMinDist, 0.001f, 0.000001f, 1.0f);
					ImGui::DragFloat("AO Rays Max. Length", &mAmbientOcclusionMaxDist, 0.01f, 0.001f, 1000.0f);
					ImGui::SliderFloat("AO Intensity", &mAmbientOcclusionFactor, 0.0f, 1.0f);
					ImGui::ColorEdit3("AO Color", glm::value_ptr(mAmbientOcclusionColor));
				}

				ImGui::End();
			});
		}

		mTimestampPool = avk::context().create_query_pool_for_timestamp_queries(
			static_cast<uint32_t>(avk::context().main_window()->number_of_frames_in_flight()) * 2
		);
	}

	void update() override
	{
		auto* triMeshGeomMgr = avk::current_composition()->element_by_type<triangle_mesh_geometry_manager>();
		assert(nullptr != triMeshGeomMgr);
		if (triMeshGeomMgr->has_updated_geometry_for_tlas())
		{
			// Getometry selection has changed => rebuild the TLAS:

			std::vector<avk::geometry_instance> activeGeometryInstances = triMeshGeomMgr->get_active_geometry_instances_for_tlas_build();

			if (!activeGeometryInstances.empty()) {
				// Get a command pool to allocate command buffers from:
				auto& commandPool = avk::context().get_command_pool_for_single_use_command_buffers(*mQueue);

				// Create a command buffer and render into the *current* swap chain image:
				auto cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

				avk::context().record({
						// We're using only one TLAS for all frames in flight. Therefore, we need to set up a barrier
						// affecting the whole queue which waits until all previous ray tracing work has completed:
						avk::sync::global_execution_barrier(avk::stage::ray_tracing_shader >> avk::stage::acceleration_structure_build),

						// ...then we can safely update the TLAS with new data:
						mTlas->build(                // We're not updating existing geometry, but we are changing the geometry => therefore, we need to perform a full rebuild.
							activeGeometryInstances, // Build only with the active geometry instances
							{}                       // Let top_level_acceleration_structure_t::build handle the staging buffer internally 
						),

						// ...and we need to ensure that the TLAS update-build has completed (also in terms of memory
						// access--not only execution) before we may continue ray tracing with that TLAS:
						avk::sync::global_memory_barrier(
							avk::stage::acceleration_structure_build  >> avk::stage::ray_tracing_shader,
							avk::access::acceleration_structure_write >> avk::access::acceleration_structure_read
						)
					})
					.into_command_buffer(cmdBfr)
					.then_submit_to(*mQueue)
					.submit();
				
				avk::context().main_window()->handle_lifetime(std::move(cmdBfr));
			}

			mTlasUpdateRequired = false;
		}

		if (avk::input().key_pressed(avk::key_code::space)) {
			// Print the current camera position
			auto pos = mQuakeCam.translation();
			LOG_INFO(std::format("Current camera position: {}", avk::to_string(pos)));
		}
		if (avk::input().key_pressed(avk::key_code::escape) || avk::context().main_window()->should_be_closed()) {
			// Stop the current composition:
			avk::current_composition()->stop();
		}
		if (avk::input().key_pressed(avk::key_code::f1)) {
			auto imguiManager = avk::current_composition()->element_by_type<avk::imgui_manager>();
			if (mQuakeCam.is_enabled()) {
				mQuakeCam.disable();
				if (nullptr != imguiManager) { imguiManager->enable_user_interaction(true); }
			}
			else {
				mQuakeCam.enable();
				if (nullptr != imguiManager) { imguiManager->enable_user_interaction(false); }
			}
		}
	}

	void render() override
	{
		auto mainWnd = avk::context().main_window();
		auto inFlightIndex = mainWnd->in_flight_index_for_frame();

		// Get a command pool to allocate command buffers from:
		auto& commandPool = avk::context().get_command_pool_for_single_use_command_buffers(*mQueue);

		// Create a command buffer and render into the *current* swap chain image:
		auto cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		// The triangle_mesh_geometry_manager has some of the data we require:
		auto* triMeshGeomMgr = avk::current_composition()->element_by_type<triangle_mesh_geometry_manager>();

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();

		const auto firstQueryIndex = static_cast<uint32_t>(inFlightIndex) * 2;
		if (mainWnd->current_frame() > mainWnd->number_of_frames_in_flight()) // otherwise we will wait forever
		{
			auto timers = mTimestampPool->get_results<uint64_t, 2>(firstQueryIndex, 2, vk::QueryResultFlagBits::eWait); // => ensure that the results are available
			mLastTraceRaysDuration = timers[1] - timers[0];
			mLastFrameDuration = timers[1] - mLastTimestamp;
			mLastTimestamp = timers[1];
		}

		avk::context().record({
				mTimestampPool->reset(firstQueryIndex, 2),
				mTimestampPool->write_timestamp(firstQueryIndex + 0, avk::stage::all_commands), // measure before trace_rays

				avk::command::bind_pipeline(mPipeline.as_reference()),
				avk::command::bind_descriptors(mPipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
					avk::descriptor_binding(0, 0, triMeshGeomMgr->combined_image_sampler_descriptor_infos()),
					avk::descriptor_binding(0, 1, triMeshGeomMgr->material_buffer()),
					avk::descriptor_binding(0, 2, avk::as_uniform_texel_buffer_views(triMeshGeomMgr->index_buffer_views())),
					avk::descriptor_binding(0, 3, avk::as_uniform_texel_buffer_views(triMeshGeomMgr->tex_coords_buffer_views())),
					avk::descriptor_binding(0, 4, avk::as_uniform_texel_buffer_views(triMeshGeomMgr->normals_buffer_views())),
					avk::descriptor_binding(1, 0, mOffscreenImageView->as_storage_image(avk::layout::general)),
					avk::descriptor_binding(2, 0, mTlas)
				})),
				avk::command::push_constants(
					mPipeline->layout(), 
					push_const_data{
						glm::vec4{mAmbientLight, 0.0f},
						glm::vec4{mLightDir, 0.0f},
						mQuakeCam.global_transformation_matrix(),
						glm::radians(mFieldOfViewForRayTracing) * 0.5f,
						0.0f, // padding
						mEnableShadows ? vk::Bool32{VK_TRUE} : vk::Bool32{VK_FALSE},
						mShadowsFactor,
						glm::vec4{ mShadowsColor, 1.0f },
						mEnableAmbientOcclusion ? vk::Bool32{VK_TRUE} : vk::Bool32{VK_FALSE},
						mAmbientOcclusionMinDist,
						mAmbientOcclusionMaxDist,
						mAmbientOcclusionFactor,
						glm::vec4{ mAmbientOcclusionColor, 1.0f }
					},
					avk::shader_type::ray_generation | avk::shader_type::closest_hit
				),

				// Do it:
				avk::command::trace_rays(
					avk::for_each_pixel(mainWnd),
					mPipeline->shader_binding_table(),
					avk::using_raygen_group_at_index(0),
					avk::using_miss_group_at_index(0),
					avk::using_hit_group_at_index(0)
				),

				mTimestampPool->write_timestamp(firstQueryIndex + 1, avk::stage::ray_tracing_shader), // measure after trace_rays

				// Wait until ray tracing has completed, then transfer the result image into the swap chain image:
				avk::sync::image_memory_barrier(mOffscreenImageView->get_image(),
					avk::stage::ray_tracing_shader >> avk::stage::copy,
					avk::access::shader_write      >> avk::access::transfer_read
				).with_layout_transition(avk::layout::general >> avk::layout::transfer_src),
				avk::sync::image_memory_barrier(mainWnd->current_backbuffer_reference().image_at(0),
					avk::stage::none               >> avk::stage::copy,
					avk::access::none              >> avk::access::transfer_write
				).with_layout_transition(avk::layout::undefined >> avk::layout::transfer_dst),

				avk::copy_image_to_another(
					mOffscreenImageView->get_image(), avk::layout::transfer_src,
					mainWnd->current_backbuffer_reference().image_at(0), avk::layout::transfer_dst
				),

				avk::sync::image_memory_barrier(mOffscreenImageView->get_image(),
					avk::stage::copy               >> avk::stage::ray_tracing_shader,
					avk::access::transfer_read     >> avk::access::shader_write
				).with_layout_transition(avk::layout::transfer_src >> avk::layout::general),
				avk::sync::image_memory_barrier(mainWnd->current_backbuffer_reference().image_at(0), // Prepare for ImGui
					avk::stage::copy               >> avk::stage::color_attachment_output,
					avk::access::transfer_write    >> avk::access::color_attachment_write
				).with_layout_transition(avk::layout::transfer_dst >> avk::layout::color_attachment_optimal),
			})
			.into_command_buffer(cmdBfr)
			.then_submit_to(*mQueue)
			// Do not start to render before the image has become available:
			.waiting_for(imageAvailableSemaphore >> avk::stage::color_attachment_output)
			.submit();

		// Submit the draw call and take care of the command buffer's lifetime:
		mainWnd->handle_lifetime(std::move(cmdBfr));
	}

private: // v== Member variables ==v

	// --------------- Some fundamental stuff -----------------

	// Our only queue where we submit command buffers to:
	avk::queue* mQueue;

	// Our only descriptor cache which stores reusable descriptor sets:
	avk::descriptor_cache mDescriptorCache;

	// ------------- Scene and model properties ---------------

	// Ambient light of our scene:
	glm::vec3 mAmbientLight = { 0.5f, 0.5f, 0.5f };

	// The direction of our single light source, which is a directional light:
	glm::vec3 mLightDir = { 0.0f, -1.0f, 0.0f };

	// ----------- Resources required for ray tracing -----------

	// We are using one single top-level acceleration structure (TLAS) to keep things simple:
	//    (We're not duplicating the TLAS per frame in flight. Instead, we
	//     are using barriers to ensure correct rendering after some data 
	//     has changed in one or multiple of the acceleration structures.)
	avk::top_level_acceleration_structure mTlas;

	// We are rendering into one single target offscreen image (Otherwise we would need multiple
	// TLAS instances, too.) to keep things simple:
	avk::image_view mOffscreenImageView;
	// (After blitting this image into one of the window's backbuffers, the GPU can 
	//  possibly achieve some parallelization of work during presentation.)

	// Thre ray tracing pipeline that renders everything into the mOffscreenImageView:
	avk::ray_tracing_pipeline mPipeline;

	// ----------------- Further invokees --------------------

	// A camera to navigate our scene, which provides us with the view matrix:
	avk::quake_camera mQuakeCam;

	// ------------------- UI settings -----------------------

	float mFieldOfViewForRayTracing = 45.0f;
	bool mEnableShadows = true;
	float mShadowsFactor = 0.5f;
	glm::vec3 mShadowsColor = glm::vec3{ 0.0f, 0.0f, 0.0f };
	bool mEnableAmbientOcclusion = true;
	float mAmbientOcclusionMinDist = 0.05f;
	float mAmbientOcclusionMaxDist = 0.25f;
	float mAmbientOcclusionFactor = 0.5f;
	glm::vec3 mAmbientOcclusionColor = glm::vec3{ 0.0f, 0.0f, 0.0f };

	// One boolean per geometry instance to tell if it shall be included in the
	// generation of the TLAS or not:
	std::vector<bool> mGeometryInstanceActive;

	bool mTlasUpdateRequired = false;

	avk::query_pool mTimestampPool;
	uint64_t mLastTimestamp = 0;
	uint64_t mLastTraceRaysDuration = 0;
	uint64_t mLastFrameDuration = 0;

}; // End of ray_query_in_ray_tracing_shaders_invokee

int main() // <== Starting point ==
{
	int result = EXIT_FAILURE;
	try {
		// Create a window and open it:
		auto mainWnd = avk::context().create_window("Ray Query in Ray Tracing Shaders - Main Window");
		mainWnd->set_resolution({ 1920, 1080 });
		mainWnd->enable_resizing(true);
		mainWnd->set_presentaton_mode(avk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		// Create one single queue to submit command buffers to:
		auto& singleQueue = avk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->set_queue_family_ownership(singleQueue.family_index());
		mainWnd->set_present_queue(singleQueue);

		// Create an instance of our main invokee:
		auto mainInvokee = ray_query_in_ray_tracing_shaders_invokee(singleQueue);
		// Create an instance of the invokee that handles our triangle mesh geometry:
		auto triMeshGeomMgrInvokee = triangle_mesh_geometry_manager(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto imguiManagerInvokee = avk::imgui_manager(singleQueue);

		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			avk::application_name("Auto-Vk-Toolkit Example: Ray Query in Ray Tracing Shaders"),
			avk::required_device_extensions()
			// We need several extensions for ray tracing:
				.add_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
				.add_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
				.add_extension(VK_KHR_RAY_QUERY_EXTENSION_NAME)
				.add_extension(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME)
				.add_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)
				.add_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)
				.add_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)
				.add_extension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME),
			[](vk::PhysicalDeviceVulkan12Features& aVulkan12Featues) {
				// Also this Vulkan 1.2 feature is required for ray tracing:
				aVulkan12Featues.setBufferDeviceAddress(VK_TRUE);
			},
			[](vk::PhysicalDeviceRayTracingPipelineFeaturesKHR& aRayTracingFeatures) {
				// Enabling the extensions is not enough, we need to activate ray tracing features explicitly here:
				aRayTracingFeatures.setRayTracingPipeline(VK_TRUE);
			},
			[](vk::PhysicalDeviceAccelerationStructureFeaturesKHR& aAccelerationStructureFeatures) {
				// ...and here:
				aAccelerationStructureFeatures.setAccelerationStructure(VK_TRUE);
			},
			[](vk::PhysicalDeviceRayQueryFeaturesKHR& aRayQueryFeatures) {
				aRayQueryFeatures.setRayQuery(VK_TRUE);
			},
			// Pass our main window to render into its frame buffers:
			mainWnd,
			// Pass the invokees that shall be invoked every frame:
			mainInvokee, triMeshGeomMgrInvokee, imguiManagerInvokee
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
	catch (avk::logic_error& e) { LOG_ERROR(std::string("Caught avk::logic_error in main(): ") + e.what()); }
	catch (avk::runtime_error& e) { LOG_ERROR(std::string("Caught avk::runtime_error in main(): ") + e.what()); }
	return result;
}
