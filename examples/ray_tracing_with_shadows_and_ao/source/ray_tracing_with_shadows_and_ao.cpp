#include <gvk.hpp>
#include <imgui.h>
#include "triangle_mesh_geometry_manager.hpp"

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
class ray_tracing_with_shadows_and_ao_invokee : public gvk::invokee
{
public: // v== gvk::invokee overrides which will be invoked by the framework ==v
	ray_tracing_with_shadows_and_ao_invokee(avk::queue& aQueue)
		: mQueue{ &aQueue }
	{}

	void initialize() override
	{
		// Create a descriptor cache that helps us to conveniently create descriptor sets,
		// which describe where shaders can find resources like buffers or images:
		mDescriptorCache = gvk::context().create_descriptor_cache();

		// Set the direction towards the light:
		mLightDir = { 0.8f, 1.0f, 0.0f };

		// Get a pointer to the main window:		
		auto* mainWnd = gvk::context().main_window();

		// Create an offscreen image to ray-trace into. It is accessed via an image view:
		const auto wdth = gvk::context().main_window()->resolution().x;
		const auto hght = gvk::context().main_window()->resolution().y;
		const auto frmt = gvk::format_from_window_color_buffer(mainWnd);
		auto offscreenImage = gvk::context().create_image(wdth, hght, frmt, 1, avk::memory_usage::device, avk::image_usage::general_storage_image);
		offscreenImage->transition_to_layout();
		mOffscreenImageView = gvk::context().create_image_view(avk::owned(offscreenImage));

		// The triangle_mesh_geometry_manager has lower execution order. Therefore, we can
		// assume that it already contains the data that we require:
		auto* triMeshGeomMgr = gvk::current_composition()->element_by_type<triangle_mesh_geometry_manager>();
		assert(nullptr != triMeshGeomMgr);

		// Initialize the TLAS (but don't build it yet)
		mTlas = gvk::context().create_top_level_acceleration_structure(
			triMeshGeomMgr->max_number_of_geometry_instances(), // <-- Specify how many geometry instances there are expected to be
			true               // <-- Allow updates since we want to have the opportunity to enable/disable some of them via the UI
		);

		// Create our ray tracing pipeline with the required configuration:
		mPipeline = gvk::context().create_ray_tracing_pipeline_for(
			// Specify all the shaders which participate in rendering in a shader binding table (the order matters):
			// In contrast to the ray_query_in_ray_tracing_shaders example, we have multiple closest hit and also
			// multiple miss shaders. When we send out the secondary rays (in first_hit_closest_hit_shader.rchit),
			// we will need to specify the offsets into this table accordingly in order to use the right shaders.
			avk::define_shader_table(
				avk::ray_generation_shader("shaders/ray_gen_shader.rgen"),
				avk::triangles_hit_group::create_with_rchit_only("shaders/first_hit_closest_hit_shader.rchit"),
				avk::triangles_hit_group::create_with_rchit_only("shaders/shadow_closest_hit_shader.rchit"),
				avk::triangles_hit_group::create_with_rchit_only("shaders/ao_closest_hit_shader.rchit"),
				avk::miss_shader("shaders/first_hit_miss_shader.rmiss"),
				avk::miss_shader("shaders/shadow_ao_miss_shader.rmiss")
			),
			// We won't need the maximum recursion depth, but why not:
			gvk::context().get_max_ray_tracing_recursion_depth(),
			// Define push constants and descriptor bindings:
			avk::push_constant_binding_data{ avk::shader_type::ray_generation | avk::shader_type::closest_hit, 0, sizeof(push_const_data) },
			avk::descriptor_binding(0, 0, triMeshGeomMgr->image_samplers()),
			avk::descriptor_binding(0, 1, triMeshGeomMgr->material_buffer()),
			avk::descriptor_binding(0, 2, avk::as_uniform_texel_buffer_views(triMeshGeomMgr->index_buffer_views())),
			avk::descriptor_binding(0, 3, avk::as_uniform_texel_buffer_views(triMeshGeomMgr->tex_coords_buffer_views())),
			avk::descriptor_binding(0, 4, avk::as_uniform_texel_buffer_views(triMeshGeomMgr->normals_buffer_views())),
			avk::descriptor_binding(1, 0, mOffscreenImageView->as_storage_image()), // Bind the offscreen image to render into as storage image
			avk::descriptor_binding(2, 0, mTlas)                                    // Bind the TLAS, s.t. we can trace rays against it
		);

		// Print the structure of our shader binding table, also displaying the offsets:
		mPipeline->print_shader_binding_table_groups();

#if ENABLE_SHADER_HOT_RELOADING_FOR_RAY_TRACING_PIPELINE || ENABLE_RESIZABLE_WINDOW
		// Create an updater:
		mUpdater.emplace();
		mPipeline.enable_shared_ownership(); // The updater needs to hold a reference to it, so we need to enable shared ownership.

#if ENABLE_SHADER_HOT_RELOADING_FOR_RAY_TRACING_PIPELINE
		mUpdater->on(gvk::shader_files_changed_event(mPipeline))
			.update(mPipeline);
#endif

#if ENABLE_RESIZABLE_WINDOW
		mOffscreenImageView.enable_shared_ownership(); // The updater needs to hold a reference to it, so we need to enable shared ownership.
		mUpdater->on(gvk::swapchain_resized_event(gvk::context().main_window()))
			.update(mOffscreenImageView, mPipeline)
			.then_on(gvk::destroying_image_view_event()) // Make sure that our descriptor cache stays cleaned up:
			.invoke([this](const avk::image_view& aImageViewToBeDestroyed) {
			auto numRemoved = mDescriptorCache.remove_sets_with_handle(aImageViewToBeDestroyed->handle());
				});
#endif
#endif

		// Add the camera to the composition (and let it handle the updates)
		mQuakeCam.set_translation({ 0.0f, 10.0f, 45.0f });
		mQuakeCam.set_perspective_projection(glm::radians(60.0f), gvk::context().main_window()->aspect_ratio(), 0.5f, 100.0f);
		gvk::current_composition()->add_element(mQuakeCam);

		// Add an "ImGui Manager" which handles the UI:
		auto imguiManager = gvk::current_composition()->element_by_type<gvk::imgui_manager>();
		if (nullptr != imguiManager) {
			imguiManager->add_callback([this]() {
				ImGui::Begin("Info & Settings");
				ImGui::SetWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
				ImGui::SetWindowSize(ImVec2(400.0f, 410.0f), ImGuiCond_FirstUseEver);

				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
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
	}

	void update() override
	{
		auto* triMeshGeomMgr = gvk::current_composition()->element_by_type<triangle_mesh_geometry_manager>();
		assert(nullptr != triMeshGeomMgr);
		if (triMeshGeomMgr->has_updated_geometry_for_tlas())
		{
			// Getometry selection has changed => rebuild the TLAS:

			std::vector<avk::geometry_instance> activeGeometryInstances = triMeshGeomMgr->get_active_geometry_instances_for_tlas_build();

			if (!activeGeometryInstances.empty()) {
				auto& commandPool = gvk::context().get_command_pool_for_single_use_command_buffers(*mQueue);
				auto cmdbfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
				cmdbfr->begin_recording();

				// We're using only one TLAS for all frames in flight. Therefore, we need to set up a barrier
				// affecting the whole queue which waits until all previous ray tracing work has completed:
				cmdbfr->establish_execution_barrier(
					avk::pipeline_stage::ray_tracing_shaders, /* -> */ avk::pipeline_stage::acceleration_structure_build
				);

				// ...then we can safely update the TLAS with new data:
				mTlas->build(                // We're not updating existing geometry, but we are changing the geometry => therefore, we need to perform a full rebuild.
					activeGeometryInstances, // Build only with the active geometry instances
					{},                      // Let top_level_acceleration_structure_t::build handle the staging buffer internally 
					avk::sync::with_barriers_into_existing_command_buffer(*cmdbfr, {}, {})
				);

				// ...and we need to ensure that the TLAS update-build has completed (also in terms of memory
				// access--not only execution) before we may continue ray tracing with that TLAS:
				cmdbfr->establish_global_memory_barrier(
					avk::pipeline_stage::acceleration_structure_build,       /* -> */ avk::pipeline_stage::ray_tracing_shaders,
					avk::memory_access::acceleration_structure_write_access, /* -> */ avk::memory_access::acceleration_structure_read_access
				);

				cmdbfr->end_recording();
				mQueue->submit(avk::referenced(cmdbfr));
				gvk::context().main_window()->handle_lifetime(avk::owned(cmdbfr));
			}

			mTlasUpdateRequired = false;
		}

		if (gvk::input().key_pressed(gvk::key_code::space)) {
			// Print the current camera position
			auto pos = mQuakeCam.translation();
			LOG_INFO(fmt::format("Current camera position: {}", gvk::to_string(pos)));
		}
		if (gvk::input().key_pressed(gvk::key_code::escape)) {
			// Stop the current composition:
			gvk::current_composition()->stop();
		}
		if (gvk::input().key_pressed(gvk::key_code::f1)) {
			auto imguiManager = gvk::current_composition()->element_by_type<gvk::imgui_manager>();
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
		auto mainWnd = gvk::context().main_window();
		auto inFlightIndex = mainWnd->in_flight_index_for_frame();

		auto& commandPool = gvk::context().get_command_pool_for_single_use_command_buffers(*mQueue);
		auto cmdbfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		cmdbfr->begin_recording();

		// The triangle_mesh_geometry_manager has some of the data we require:
		auto* triMeshGeomMgr = gvk::current_composition()->element_by_type<triangle_mesh_geometry_manager>();

		cmdbfr->bind_pipeline(avk::const_referenced(mPipeline));
		cmdbfr->bind_descriptors(mPipeline->layout(), mDescriptorCache.get_or_create_descriptor_sets({
			avk::descriptor_binding(0, 0, triMeshGeomMgr->image_samplers()),
			avk::descriptor_binding(0, 1, triMeshGeomMgr->material_buffer()),
			avk::descriptor_binding(0, 2, avk::as_uniform_texel_buffer_views(triMeshGeomMgr->index_buffer_views())),
			avk::descriptor_binding(0, 3, avk::as_uniform_texel_buffer_views(triMeshGeomMgr->tex_coords_buffer_views())),
			avk::descriptor_binding(0, 4, avk::as_uniform_texel_buffer_views(triMeshGeomMgr->normals_buffer_views())),
			avk::descriptor_binding(1, 0, mOffscreenImageView->as_storage_image()),
			avk::descriptor_binding(2, 0, mTlas)
			}));

		// Set the push constants:
		auto pushConstantsForThisDrawCall = push_const_data{
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
		};
		cmdbfr->handle().pushConstants(mPipeline->layout_handle(), vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR, 0, sizeof(pushConstantsForThisDrawCall), &pushConstantsForThisDrawCall);

		// Do it:
		cmdbfr->trace_rays(
			gvk::for_each_pixel(mainWnd),
			mPipeline->shader_binding_table(),
			avk::using_raygen_group_at_index(0),
			avk::using_miss_group_at_index(0),
			avk::using_hit_group_at_index(0)
		);

		// Sync ray tracing with transfer:
		cmdbfr->establish_global_memory_barrier(
			avk::pipeline_stage::ray_tracing_shaders, avk::pipeline_stage::transfer,
			avk::memory_access::shader_buffers_and_images_write_access, avk::memory_access::transfer_read_access
		);

		avk::copy_image_to_another(
			mOffscreenImageView->get_image(),
			mainWnd->current_backbuffer()->image_at(0),
			avk::sync::with_barriers_into_existing_command_buffer(*cmdbfr, {}, {})
		);

		// Make sure to properly sync with ImGui manager which comes afterwards (it uses a graphics pipeline):
		cmdbfr->establish_global_memory_barrier(
			avk::pipeline_stage::transfer, avk::pipeline_stage::color_attachment_output,
			avk::memory_access::transfer_write_access, avk::memory_access::color_attachment_write_access
		);

		cmdbfr->end_recording();

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();

		// Submit the draw call and take care of the command buffer's lifetime:
		mQueue->submit(cmdbfr, imageAvailableSemaphore);
		mainWnd->handle_lifetime(avk::owned(cmdbfr));
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
	gvk::quake_camera mQuakeCam;

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

}; // End of ray_tracing_with_shadows_and_ao_invokee

int main() // <== Starting point ==
{
	try {
		// Create a window and open it:
		auto mainWnd = gvk::context().create_window("Real-Time Ray Tracing with Shadows and AO - Main Window");
		mainWnd->set_resolution({ 1920, 1080 });
		mainWnd->enable_resizing(true);
		mainWnd->set_presentaton_mode(gvk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		// Create one single queue to submit command buffers to:
		auto& singleQueue = gvk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->add_queue_family_ownership(singleQueue);
		mainWnd->set_present_queue(singleQueue);
		// ... pass the queue to the constructors of the invokees:

		// Create an instance of our main invokee:
		auto mainInvokee = ray_tracing_with_shadows_and_ao_invokee(singleQueue);
		// Create an instance of the invokee that handles our triangle mesh geometry:
		auto triMeshGeomMgrInvokee = triangle_mesh_geometry_manager();
		// Create another element for drawing the UI with ImGui
		auto imguiManagerInvokee = gvk::imgui_manager(singleQueue);

		// Launch the render loop in 5.. 4.. 3.. 2.. 1.. 
		gvk::start(
			gvk::application_name("Real-Time Ray Tracing with Shadows and AO"),
			gvk::required_device_extensions()
			// We need several extensions for ray tracing:
			.add_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
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
			// Pass our main window to render into its frame buffers:
			mainWnd,
			// Pass the invokees that shall be invoked every frame:
			mainInvokee, triMeshGeomMgrInvokee, imguiManagerInvokee
		);
	}
	catch (gvk::logic_error& e) { LOG_ERROR(std::string("Caught gvk::logic_error in main(): ") + e.what()); }
	catch (gvk::runtime_error& e) { LOG_ERROR(std::string("Caught gvk::runtime_error in main(): ") + e.what()); }
	catch (avk::logic_error& e) { LOG_ERROR(std::string("Caught avk::logic_error in main(): ") + e.what()); }
	catch (avk::runtime_error& e) { LOG_ERROR(std::string("Caught avk::runtime_error in main(): ") + e.what()); }
}
