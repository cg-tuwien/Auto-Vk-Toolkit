#include <gvk.hpp>
#include <imgui.h>

class ray_tracing_basic_usage_app : public gvk::invokee
{
	struct push_const_data {
		glm::mat4 mCameraTransform;
		glm::vec4 mLightDir;
	};

public: // v== avk::invokee overrides which will be invoked by the framework ==v
	ray_tracing_basic_usage_app(avk::queue& aQueue)
		: mQueue{ &aQueue }
	{}
	
	void initialize() override
	{
		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = gvk::context().create_descriptor_cache();
		
		// Load an ORCA scene from file:
		auto orca = gvk::orca_scene_t::load_from_file("assets/sponza_duo.fscene");
		// Iterate over all models, all model instances, and all meshes, and create bottom level acceleration structures for each one of them:
		for (const auto& modelData : orca->models()) {
			for (const auto& modelInstance : modelData.mInstances) {
				const auto& model = modelData.mLoadedModel;
				auto meshIndices = model->select_all_meshes();
				auto [vtxBfr, idxBfr] = gvk::create_vertex_and_index_buffers({ gvk::make_models_and_meshes_selection(model, meshIndices) }, vk::BufferUsageFlagBits::eShaderDeviceAddressKHR);
				auto blas = gvk::context().create_bottom_level_acceleration_structure({ 
					avk::acceleration_structure_size_requirements::from_buffers(avk::vertex_index_buffer_pair{ vtxBfr, idxBfr })
				}, false);
				blas->build({ avk::vertex_index_buffer_pair{ vtxBfr, idxBfr } });
				mGeometryInstances.push_back(
					gvk::context().create_geometry_instance(blas)
						.set_transform_column_major(gvk::to_array( gvk::matrix_from_transforms(modelInstance.mTranslation, glm::quat(modelInstance.mRotation), modelInstance.mScaling) ))
				);
				mBLASs.push_back(std::move(blas));
			}
		}

		auto mainWnd = gvk::context().main_window();
		const auto numFramesInFlight = mainWnd->number_of_frames_in_flight();
		for (int i = 0; i < numFramesInFlight; ++i) {

			// Create top level acceleration structures, one for each frame in flight:
			auto tlas = gvk::context().create_top_level_acceleration_structure( mGeometryInstances.size() );
			tlas->build( mGeometryInstances );
			mTLAS.push_back( std::move(tlas) );

			// Create offscreen image views to ray-trace into, one for each frame in flight:
			const auto wdth = mainWnd->resolution().x;
			const auto hght = mainWnd->resolution().y;
			const auto frmt = gvk::format_from_window_color_buffer(gvk::context().main_window());

			mOffscreenImageViews.emplace_back(
				gvk::context().create_image_view(
					gvk::context().create_image(wdth, hght, frmt, 1, avk::memory_usage::device, avk::image_usage::general_storage_image)
				)
			);

			mOffscreenImageViews.back()->get_image().transition_to_layout({}, avk::sync::with_barriers(mainWnd->command_buffer_lifetime_handler()));
			assert((mOffscreenImageViews.back()->config().subresourceRange.aspectMask & vk::ImageAspectFlagBits::eColor) == vk::ImageAspectFlagBits::eColor);

		}

		// Create our ray tracing pipeline with the required configuration:
		mPipeline = gvk::context().create_ray_tracing_pipeline_for(
			avk::define_shader_table(
				avk::ray_generation_shader("shaders/ray_generation_shader.rgen"),
				avk::triangles_hit_group::create_with_rchit_only("shaders/closest_hit_shader.rchit"),
				avk::miss_shader("shaders/miss_shader.rmiss")
			),
			// Define push constants and descriptor bindings:
			avk::push_constant_binding_data { avk::shader_type::ray_generation | avk::shader_type::closest_hit, 0, sizeof(push_const_data) },
			avk::descriptor_binding(0, 0, mOffscreenImageViews[0]->as_storage_image()), // Just take any, this is just to define the layout
			avk::descriptor_binding(1, 0, mTLAS[0])				 // Just take any, this is just to define the layout
		);

		mPipeline->print_shader_binding_table_groups();

		// Add the camera to the composition (and let it handle the updates)
		mQuakeCam.set_translation({ 0.0f, 0.0f, 0.0f });
		mQuakeCam.set_perspective_projection(glm::radians(60.0f), mainWnd->aspect_ratio(), 0.5f, 100.0f);
		//mQuakeCam.set_orthographic_projection(-5, 5, -5, 5, 0.5, 100);
		gvk::current_composition()->add_element(mQuakeCam);

		auto imguiManager = gvk::current_composition()->element_by_type<gvk::imgui_manager>();
		if(nullptr != imguiManager) {
			imguiManager->add_callback([this](){
		        ImGui::Begin("Info & Settings");
				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
				ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), "[F1]: Toggle input-mode");
				ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), " (UI vs. scene navigation)");
				ImGui::DragFloat3("Light Direction", glm::value_ptr(mLightDir), 0.005f, -1.0f, 1.0f);
				mLightDir = glm::normalize(mLightDir);
				ImGui::End();
			});
		}
	}

	void update() override
	{
		static int counter = 0;
		if (++counter == 4) {
			auto current = std::chrono::high_resolution_clock::now();
			auto time_span = current - mInitTime;
			auto int_min = std::chrono::duration_cast<std::chrono::minutes>(time_span).count();
			auto int_sec = std::chrono::duration_cast<std::chrono::seconds>(time_span).count();
			auto fp_ms = std::chrono::duration<double, std::milli>(time_span).count();
			printf("Time from init to fourth frame: %d min, %lld sec %lf ms\n", int_min, int_sec - static_cast<decltype(int_sec)>(int_min) * 60, fp_ms - 1000.0 * int_sec);
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
		cmdbfr->bind_pipeline(avk::const_referenced(mPipeline));
		cmdbfr->bind_descriptors(mPipeline->layout(), mDescriptorCache.get_or_create_descriptor_sets({  
			avk::descriptor_binding(0, 0, mOffscreenImageViews[inFlightIndex]->as_storage_image()),
			avk::descriptor_binding(1, 0, mTLAS[inFlightIndex])
		}));

		// Set the push constants:
		auto pushConstantsForThisDrawCall = push_const_data { 
			mQuakeCam.global_transformation_matrix(),
			glm::vec4{mLightDir, 0.0f}
		};
		cmdbfr->handle().pushConstants(mPipeline->layout_handle(), vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR, 0, sizeof(pushConstantsForThisDrawCall), &pushConstantsForThisDrawCall);

		cmdbfr->trace_rays(
			gvk::for_each_pixel(mainWnd),
			mPipeline->shader_binding_table(),
			avk::using_raygen_group_at_index(0),
			avk::using_miss_group_at_index(0),
			avk::using_hit_group_at_index(0)
		);
		
		// Sync ray tracing with transfer:
		cmdbfr->establish_global_memory_barrier(
			avk::pipeline_stage::ray_tracing_shaders,                       avk::pipeline_stage::transfer,
			avk::memory_access::shader_buffers_and_images_write_access,     avk::memory_access::transfer_read_access
		);

		avk::copy_image_to_another(
			mOffscreenImageViews[inFlightIndex]->get_image(), 
			mainWnd->current_backbuffer()->image_at(0), 
			avk::sync::with_barriers_into_existing_command_buffer(*cmdbfr, {}, {})
		);

		// Make sure to properly sync with ImGui manager which comes afterwards (it uses a graphics pipeline):
		cmdbfr->establish_global_memory_barrier(
			avk::pipeline_stage::transfer,                                  avk::pipeline_stage::color_attachment_output,
			avk::memory_access::transfer_write_access,                      avk::memory_access::color_attachment_write_access
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
	std::chrono::high_resolution_clock::time_point mInitTime;

	avk::queue* mQueue;
	avk::descriptor_cache mDescriptorCache;
	
	// Multiple BLAS, concurrently used by all the (three?) TLASs:
	std::vector<avk::bottom_level_acceleration_structure> mBLASs;
	// Geometry instance data which store the instance data per BLAS
	std::vector<avk::geometry_instance> mGeometryInstances;
	// Multiple TLAS, one for each frame in flight:
	std::vector<avk::top_level_acceleration_structure> mTLAS;

	std::vector<avk::image_view> mOffscreenImageViews;

	glm::vec3 mLightDir = {0.0f, -1.0f, 0.0f};
	
	avk::ray_tracing_pipeline mPipeline;
	gvk::quake_camera mQuakeCam;

}; // ray_tracing_basic_usage_app

int main() // <== Starting point ==
{
	try {

		// Create a window and open it
		auto mainWnd = gvk::context().create_window("Real-Time Ray Tracing - Basic Usage Example");
		mainWnd->set_resolution({ 1920, 1080 });
		mainWnd->set_presentaton_mode(gvk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = gvk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->add_queue_family_ownership(singleQueue);
		mainWnd->set_present_queue(singleQueue);
		
		// Create an instance of our main avk::element which contains all the functionality:
		auto app = ray_tracing_basic_usage_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = gvk::imgui_manager(singleQueue);

		// GO:
		gvk::start(
			gvk::application_name("Gears-Vk + Auto-Vk Example: Real-Time Ray Tracing - Basic Usage Example"),
#if VK_HEADER_VERSION >= 162
			gvk::required_device_extensions()
				.add_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
				.add_extension(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME)
				.add_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)
				.add_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)
				.add_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)
				.add_extension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME),
			[](vk::PhysicalDeviceVulkan12Features& aVulkan12Featues){
				aVulkan12Featues.setBufferDeviceAddress(VK_TRUE);
			},
			[](vk::PhysicalDeviceRayTracingPipelineFeaturesKHR& aRayTracingFeatures) {
				aRayTracingFeatures.setRayTracingPipeline(VK_TRUE);
			},
			[](vk::PhysicalDeviceAccelerationStructureFeaturesKHR& aAccelerationStructureFeatures) {
				aAccelerationStructureFeatures.setAccelerationStructure(VK_TRUE);
			},
#else 
			gvk::required_device_extensions()
				.add_extension(VK_KHR_RAY_TRACING_EXTENSION_NAME)
				.add_extension(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME)
				.add_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)
				.add_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)
				.add_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)
				.add_extension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME),
			[](vk::PhysicalDeviceVulkan12Features& aVulkan12Featues) {
				aVulkan12Featues.setBufferDeviceAddress(VK_TRUE);
			},
			[](vk::PhysicalDeviceRayTracingFeaturesKHR& aRayTracingFeatures) {
				aRayTracingFeatures.setRayTracing(VK_TRUE);
			},
#endif
			mainWnd,
			app,
			ui
		);
	}
	catch (gvk::logic_error&) {}
	catch (gvk::runtime_error&) {}
	catch (avk::logic_error&) {}
	catch (avk::runtime_error&) {}
}


