#include <gvk.hpp>
#include <imgui.h>

class ray_tracing_custom_intersection_app : public gvk::invokee
{
	struct transformation_matrices {
		glm::mat4 mCameraTransform;
	};

	// Define a struct for our vertex input data:
	struct Vertex {
	    glm::vec3 mPosition;
	    glm::vec3 mColor;
	};

public: // v== avk::invokee overrides which will be invoked by the framework ==v
	ray_tracing_custom_intersection_app(avk::queue& aQueue)
		: mQueue{ &aQueue }
	{}


	void set_bottom_level_aabb_data()
	{
		mAabbs[0] = VkAabbPositionsKHR{ /* min: */ -0.5f, -0.5f, -0.5f, /* max: */  0.5f,  0.5f,  0.5f };
		mAabbs[1] = VkAabbPositionsKHR{ /* min: */  1.0f,  1.0f,  1.0f, /* max: */  3.0f,  3.0f,  3.0f };
		mAabbs[2] = VkAabbPositionsKHR{ /* min: */ -3.0f, -2.0f, -1.0f, /* max: */ -2.0f, -1.0f,  0.0f };
	}

	void add_geometry_instances_for_bottom_level_acc_structure(size_t aIndex, const avk::bottom_level_acceleration_structure& aBlas, glm::vec3 aTranslation) {
		mTranslations[aIndex + 0] =  aTranslation;
		mTranslations[aIndex + 1] = -aTranslation;
		mGeometryInstances.emplace_back( gvk::context().create_geometry_instance( aBlas ).set_transform_column_major(gvk::to_array(glm::translate(glm::mat4{1.0f}, mTranslations[aIndex + 0]))) );
		mGeometryInstances.emplace_back( gvk::context().create_geometry_instance( aBlas ).set_transform_column_major(gvk::to_array(glm::translate(glm::mat4{1.0f}, mTranslations[aIndex + 1]))) );
	}

	void set_bottom_level_pyramid_data()
	{
		mPyramidVertices = {
			// pyramid front
			{{ 0.0f, -0.5f, 0.5f},  {1.0f, 0.0f, 0.0f}},
			{{ 0.3f,  0.5f, 0.2f},  {0.5f, 0.5f, 0.5f}},
			{{-0.3f,  0.5f, 0.2f},  {0.5f, 0.5f, 0.5f}},
			// pyramid right
			{{ 0.0f, -0.5f, 0.5f},  {1.0f, 0.0f, 0.0f}},
			{{ 0.3f,  0.5f, 0.8f},  {0.6f, 0.6f, 0.6f}},
			{{ 0.3f,  0.5f, 0.2f},  {0.6f, 0.6f, 0.6f}},
			// pyramid back
			{{ 0.0f, -0.5f, 0.5f},  {1.0f, 0.0f, 0.0f}},
			{{-0.3f,  0.5f, 0.8f},  {0.5f, 0.5f, 0.5f}},
			{{ 0.3f,  0.5f, 0.8f},  {0.5f, 0.5f, 0.5f}},
			// pyramid left
			{{ 0.0f, -0.5f, 0.5f},  {1.0f, 0.0f, 0.0f}},
			{{-0.3f,  0.5f, 0.2f},  {0.4f, 0.4f, 0.4f}},
			{{-0.3f,  0.5f, 0.8f},  {0.4f, 0.4f, 0.4f}},
		};
		mPyramidIndices = {
			 0, 1, 2,  3, 4, 5,  6, 7, 8,  9, 10, 11
		};
	}

	void build_pyramid_buffers()
	{
		auto& vtxBfr = mPyramidVertexBuffers.emplace_back(gvk::context().create_buffer(
			avk::memory_usage::host_visible,
#if VK_HEADER_VERSION >= 162
#else
			vk::BufferUsageFlagBits::eRayTracingKHR |
#endif
			vk::BufferUsageFlagBits::eShaderDeviceAddressKHR,
			avk::vertex_buffer_meta::create_from_data(mPyramidVertices).describe_member(&Vertex::mPosition, avk::content_description::position),
			avk::read_only_input_to_acceleration_structure_builds_buffer_meta::create_from_data(mPyramidVertices)
		));
		vtxBfr->fill(mPyramidVertices.data(), 0, avk::sync::wait_idle());
		
		auto& idxBfr = mPyramidIndexBuffers.emplace_back( gvk::context().create_buffer(
			avk::memory_usage::host_visible, 
#if VK_HEADER_VERSION >= 162
#else
			vk::BufferUsageFlagBits::eRayTracingKHR |
#endif
			vk::BufferUsageFlagBits::eShaderDeviceAddressKHR,
			avk::index_buffer_meta::create_from_data(mPyramidIndices),
			avk::read_only_input_to_acceleration_structure_builds_buffer_meta::create_from_data(mPyramidIndices)
		));
		idxBfr->fill(mPyramidIndices.data(), 0, avk::sync::wait_idle());
	}

	void initialize() override
	{
		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = gvk::context().create_descriptor_cache();
		
		auto* mainWnd = gvk::context().main_window();
		auto fif = static_cast<size_t>(mainWnd->number_of_frames_in_flight());
		for (size_t i = 0; i < fif; ++i) {
			// ------------ Add AABBs to bottom-level acceleration structures: --------------
			if (0 == i /* only once */ ) {
				set_bottom_level_aabb_data();
			}
			
			auto& bLast = mBLAS.emplace_back();
			auto& tLast = mTLAS.emplace_back();
			
			for (size_t j=0; j < 3; ++j) {
				bLast[j] = gvk::context().create_bottom_level_acceleration_structure(
					{ avk::acceleration_structure_size_requirements::from_aabbs(1u) }, 
					true /* allow updates */
				);
				bLast[j]->build({ mAabbs[j] });

				if (0 == i) {
					add_geometry_instances_for_bottom_level_acc_structure(2*j, bLast[j], glm::vec3{ 0.0f, 0.0f, -3.0f });
				}
			}

			// ------------ Add pyramid to bottom-level acceleration structures: -------------
			if (0 == i /* only once */ ) {
				// Create the data:
				set_bottom_level_pyramid_data();
			}
			// Create the buffers:
			build_pyramid_buffers();

			// Put data into an acceleration structure:
			const size_t pyrOffset = 3;
			for (size_t j=0; j < 2; ++j) {
				bLast[pyrOffset + j] = gvk::context().create_bottom_level_acceleration_structure(
					{ avk::acceleration_structure_size_requirements::from_buffers( avk::vertex_index_buffer_pair{ mPyramidVertexBuffers[i], mPyramidIndexBuffers[i] } ) },
					true /* allow updates */
				);
				bLast[pyrOffset + j]->build({ avk::vertex_index_buffer_pair{ mPyramidVertexBuffers[i], mPyramidIndexBuffers[i] } });
			}
			
			if (0 == i /* only once */ ) {
				add_geometry_instances_for_bottom_level_acc_structure(2*pyrOffset, bLast[pyrOffset], glm::vec3{ 0.0f, -3.0f, 0.0f });
			}

			// ----------- Build the top-level acceleration structure (for this frame in flight): -------------
			assert (8 == mGeometryInstances.size());
			tLast = gvk::context().create_top_level_acceleration_structure(static_cast<uint32_t>(mGeometryInstances.size()));
			tLast->build(mGeometryInstances);
		}
		
		// Create offscreen image views to ray-trace into, one for each frame in flight:
		const auto wdth = mainWnd->resolution().x;
		const auto hght = mainWnd->resolution().y;
		const auto frmt = gvk::format_from_window_color_buffer(mainWnd);
		for (decltype(fif) i = 0; i < fif; ++i) {
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
				avk::ray_generation_shader("shaders/rt_aabb.rgen"),
				avk::procedural_hit_group::create_with_rint_and_rchit("shaders/rt_aabb.rint", "shaders/rt_aabb.rchit"),
				avk::procedural_hit_group::create_with_rint_and_rchit("shaders/rt_aabb.rint", "shaders/rt_aabb_secondary.rchit"),
				avk::miss_shader("shaders/rt_aabb.rmiss"),
				avk::miss_shader("shaders/rt_aabb_secondary.rmiss")
			),
			gvk::context().get_max_ray_tracing_recursion_depth(),
			// Define push constants and descriptor bindings:
			avk::push_constant_binding_data { avk::shader_type::ray_generation, 0, sizeof(transformation_matrices) },
			avk::descriptor_binding(0, 0, mOffscreenImageViews[0]->as_storage_image()), // Just take any, this is just to define the layout
			avk::descriptor_binding(1, 0, mTLAS[0])                                     // Just take any, this is just to define the layout
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

				static std::vector<float> accum; // accumulate (then average) 10 frames
				accum.push_back(ImGui::GetIO().Framerate);
				static std::vector<float> values;
				if (accum.size() == 10) {
					values.push_back(std::accumulate(std::begin(accum), std::end(accum), 0.0f) / 10.0f);
					accum.clear();
				}
		        if (values.size() > 90) { // Display up to 90(*10) history frames
			        values.erase(values.begin());
		        }
	            ImGui::PlotLines("FPS", values.data(), static_cast<int>(values.size()), 0, nullptr, 0.0f, 1500.0f, ImVec2(0.0f, 150.0f));
				
				ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), "[F1]: Toggle input-mode");
				ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), " (UI vs. scene navigation)");

				ImGui::Separator();
				ImGui::TextColored(ImVec4(0.f, 0.8f, 0.6f, 1.f), "Modify Bottom Level Acceleration Structures:");
				for (int i=0; i < mAabbs.size(); ++i) {
					ImGui::DragFloat3(fmt::format("AABB[{}].min", i).c_str(), *reinterpret_cast<float(*)[3]>(&mAabbs[i].minX), 0.01f);
					ImGui::DragFloat3(fmt::format("AABB[{}].max", i).c_str(), *reinterpret_cast<float(*)[3]>(&mAabbs[i].maxX), 0.01f);
				}
				ImGui::DragFloat3("Pyramid Spire", *reinterpret_cast<float(*)[3]>(&mPyramidVertices[0].mPosition), 0.01f);

				ImGui::Separator();
				ImGui::TextColored(ImVec4(0.8f, 0.0f, 0.6f, 1.f), "Modify Top Level Acceleration Structures:");
				for (int i=0; i < mTranslations.size(); ++i) {
					ImGui::DragFloat3(fmt::format("Instance[{}].translation", i).c_str(), *reinterpret_cast<float(*)[3]>(&mTranslations[i]), 0.01f);
				}

				ImGui::Separator();

				ImGui::Checkbox("Update Bottom Level Acceleration Structures", &mUpdateBlas);
				ImGui::Checkbox("Update Top Level Acceleration Structures", &mUpdateTlas);
				
				ImGui::End();
			});
		}
	}

	void update() override
	{
		auto* mainWnd = gvk::context().main_window();

		static gvk::window::frame_id_t updateBlasUntilFrame = -1;
		static gvk::window::frame_id_t updateTlasUntilFrame = -1;
		if (mUpdateBlas) {
			updateBlasUntilFrame = mainWnd->current_frame() + mainWnd->number_of_frames_in_flight() - 1;
		}
		if (mUpdateTlas) {
			updateTlasUntilFrame = mainWnd->current_frame() + mainWnd->number_of_frames_in_flight() - 1;
		}
		
		if (mUpdateBlas || mainWnd->current_frame() <= updateBlasUntilFrame) {
			const auto inFlightIndex = mainWnd->in_flight_index_for_frame();
			for (size_t i=0; i < mBLAS[inFlightIndex].size(); ++i) {
				if (i < 3) { // 3 AABBs
					mBLAS[inFlightIndex][i]->build({ mAabbs[i] }, {}, avk::sync::with_barriers(mainWnd->command_buffer_lifetime_handler()));
				}
				else { // 1 triangle mesh
					for (int i=3; i < mPyramidVertices.size(); i+=3) {
						mPyramidVertices[i].mPosition = mPyramidVertices[0].mPosition;
					}
					mPyramidVertexBuffers[inFlightIndex]->fill(mPyramidVertices.data(), 0, avk::sync::wait_idle());
					mPyramidIndexBuffers[inFlightIndex]->fill(mPyramidIndices.data(), 0, avk::sync::wait_idle());
					mBLAS[inFlightIndex][i]->build({ avk::vertex_index_buffer_pair{ mPyramidVertexBuffers[inFlightIndex], mPyramidIndexBuffers[inFlightIndex] } });
					//                         ^  switch to update() once the (annoying) validation layer error message is fixed
				}
			}
		}

		if (mUpdateTlas || mainWnd->current_frame() <= updateTlasUntilFrame) {
			const auto inFlightIndex = mainWnd->in_flight_index_for_frame();
			for (size_t i=0; i < mGeometryInstances.size(); ++i) {
				mGeometryInstances[i].mTransform.matrix[0][3] = mTranslations[i][0];
				mGeometryInstances[i].mTransform.matrix[1][3] = mTranslations[i][1];
				mGeometryInstances[i].mTransform.matrix[2][3] = mTranslations[i][2];
			}
			mTLAS[inFlightIndex]->build(mGeometryInstances, {}, avk::sync::with_barriers(mainWnd->command_buffer_lifetime_handler()));
			//                      ^  switch to update() once the (annoying) validation layer error message is fixed
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
		cmdbfr->bind_descriptors(mPipeline->layout(),  mDescriptorCache.get_or_create_descriptor_sets({
			avk::descriptor_binding(0, 0, mOffscreenImageViews[inFlightIndex]->as_storage_image()),
			avk::descriptor_binding(1, 0, mTLAS[inFlightIndex])
		}));

		// Set the push constants:
		auto pushConstantsForThisDrawCall = transformation_matrices { 
			mQuakeCam.global_transformation_matrix()
		};
		cmdbfr->handle().pushConstants(mPipeline->layout_handle(), vk::ShaderStageFlagBits::eRaygenKHR, 0, sizeof(pushConstantsForThisDrawCall), &pushConstantsForThisDrawCall);

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
	avk::queue* mQueue;
	avk::descriptor_cache mDescriptorCache;
	std::vector<avk::image_view> mOffscreenImageViews;
	avk::ray_tracing_pipeline mPipeline;
	gvk::quake_camera mQuakeCam;

	// Bottom level acceleration structure data:
	std::array<VkAabbPositionsKHR, 3> mAabbs;
	std::vector<Vertex> mPyramidVertices;
	std::vector<uint16_t> mPyramidIndices;

	// Bottom level acceleration structures (3 AABBs + 1 pyramid) -- and all of that per frame in flight.
	std::vector<avk::buffer> mPyramidVertexBuffers;
	std::vector<avk::buffer> mPyramidIndexBuffers;
	std::vector<std::array<avk::bottom_level_acceleration_structure, 5>> mBLAS;

	// Geometry instance data per every bottom level acceleration structure (3 AABBs + 1 pyramid) x2 (i.e. 2 instances each)
	std::vector<avk::geometry_instance> mGeometryInstances;
	
	// Top level acceleration structure containing 4x2 geometry instances -- per frame in flight.
	std::vector<avk::top_level_acceleration_structure> mTLAS;

	// Settings from the UI:
	bool mUpdateBlas = false;
	bool mUpdateTlas = false;
	std::array<glm::vec3, 8> mTranslations; // Each AABB x2 + two pyramids
	
}; // ray_tracing_custom_intersection_app

int main() // <== Starting point ==
{
	try {
		// Create a window and open it
		auto mainWnd = gvk::context().create_window("Real-Time Ray Tracing - Custom Intersection Example");
		mainWnd->set_resolution({ 1920, 1080 });
		mainWnd->set_presentaton_mode(gvk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = gvk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->add_queue_family_ownership(singleQueue);
		mainWnd->set_present_queue(singleQueue);
		
		// Create an instance of our main avk::element which contains all the functionality:
		auto app = ray_tracing_custom_intersection_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = gvk::imgui_manager(singleQueue);

		// GO:
		gvk::start(
			gvk::application_name("Gears-Vk + Auto-Vk Example: Real-Time Ray Tracing - Custom Intersection Example"),
#if VK_HEADER_VERSION >= 162
			gvk::required_device_extensions()
				.add_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
				.add_extension(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME)
				.add_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)
				.add_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)
				.add_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)
				.add_extension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME),
			[](vk::PhysicalDeviceVulkan12Features& aVulkan12Featues) {
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
