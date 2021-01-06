#include <gvk.hpp>
#include <imgui.h>

class ray_tracing_triangle_meshes_app : public gvk::invokee
{
	struct push_const_data {
		glm::mat4 mCameraTransform;
		glm::vec4 mLightDir;
	};

public: // v== xk::invokee overrides which will be invoked by the framework ==v
	ray_tracing_triangle_meshes_app(avk::queue& aQueue)
		: mQueue{ &aQueue }
	{}
	
	void initialize() override
	{
		mInitTime = std::chrono::high_resolution_clock::now();
		mLightDir = { 0.8f, 1.0f, 0.0f };
		auto* mainWnd = gvk::context().main_window();
		
		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = gvk::context().create_descriptor_cache();
		
		// Load an ORCA scene from file:
		auto orca = gvk::orca_scene_t::load_from_file("assets/sponza_duo.fscene");
		// Get all the different materials from the whole scene:
		auto distinctMaterialsOrca = orca->distinct_material_configs_for_all_models();
		
		// The following loop gathers all the vertex and index data PER MATERIAL and constructs the buffers and materials.
		std::vector<gvk::material_config> allMatConfigs;
		for (const auto& pair : distinctMaterialsOrca) {
			auto it = std::find(std::begin(allMatConfigs), std::end(allMatConfigs), pair.first);
			allMatConfigs.push_back(pair.first); // pair.first = material config
			auto matIndex = allMatConfigs.size() - 1;

			// The data in distinctMaterialsOrca also references all the models and submesh-indices (at pair.second) which have a specific material (pair.first) 
			for (const auto& indices : pair.second) { // pair.second = all models and meshes which use the material config (i.e. pair.first)
				// However, we have to pay attention to the specific model's scene-properties,...
				auto& modelData = orca->model_at_index(indices.mModelIndex);
				// ... specifically, to its instances:
				// (Generally, we can't combine the vertex data in this case with the vertex
				//  data of other models if we have to draw multiple instances; because in 
				//  the case of multiple instances, only the to-be-instanced geometry must
				//  be accessible independently of the other geometry.
				//  Therefore, in this example, we take the approach of building separate 
				//  buffers for everything which could potentially be instanced.)

				// Get a buffer containing all positions, and one containing all indices for all submeshes with this material
				auto [positionsBuffer, indicesBuffer] = gvk::create_vertex_and_index_buffers(
					{ gvk::make_models_and_meshes_selection(modelData.mLoadedModel, indices.mMeshIndices) }, 
#if VK_HEADER_VERSION >= 162
#else
					vk::BufferUsageFlagBits::eRayTracingKHR |
#endif
					vk::BufferUsageFlagBits::eShaderDeviceAddressKHR,
					avk::sync::with_barriers(mainWnd->command_buffer_lifetime_handler())
				);
				
				// Get a buffer containing all texture coordinates for all submeshes with this material
				auto bufferViewIndex = static_cast<uint32_t>(mTexCoordBufferViews.size());
				auto texCoordsData = gvk::get_2d_texture_coordinates({ gvk::make_models_and_meshes_selection(modelData.mLoadedModel, indices.mMeshIndices) }, 0);
				auto texCoordsTexelBuffer = gvk::context().create_buffer(
					avk::memory_usage::device, 
#if VK_HEADER_VERSION >= 162
#else
					vk::BufferUsageFlagBits::eRayTracingKHR |
#endif
					vk::BufferUsageFlagBits::eShaderDeviceAddressKHR,
					avk::uniform_texel_buffer_meta::create_from_data(texCoordsData)
						.describe_only_member(texCoordsData[0])
				);
				texCoordsTexelBuffer->fill(
					texCoordsData.data(), 0,
					avk::sync::with_barriers(mainWnd->command_buffer_lifetime_handler())
				);
				mTexCoordBufferViews.push_back( gvk::context().create_buffer_view(avk::owned(texCoordsTexelBuffer)) );

				// The following call is quite redundant => TODO: optimize!
				auto [positionsData, indicesData] = gvk::get_vertices_and_indices({ gvk::make_models_and_meshes_selection(modelData.mLoadedModel, indices.mMeshIndices) });
				auto indexTexelBuffer = gvk::context().create_buffer(
					avk::memory_usage::device, 
#if VK_HEADER_VERSION >= 162
#else
					vk::BufferUsageFlagBits::eRayTracingKHR |
#endif
					vk::BufferUsageFlagBits::eShaderDeviceAddressKHR,
					avk::uniform_texel_buffer_meta::create_from_data(indicesData)
						.set_format<glm::uvec3>() // Combine 3 consecutive elements to one unit
				);
				indexTexelBuffer->fill(
					indicesData.data(), 0,
					avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler())
				);
				mIndexBufferViews.push_back( gvk::context().create_buffer_view(avk::owned(indexTexelBuffer)) );

				// Create one bottom level acceleration structure per model
				auto blas = gvk::context().create_bottom_level_acceleration_structure({ 
					avk::acceleration_structure_size_requirements::from_buffers( avk::vertex_index_buffer_pair{ positionsBuffer, indicesBuffer } )
				}, true);
				// Enable shared ownership because we'll have one TLAS per frame in flight, each one referencing the SAME BLASs
				// (But that also means that we may not modify the BLASs. They must stay the same, otherwise concurrent access will fail.)
				blas.enable_shared_ownership();
				mBLASs.push_back(blas); // No need to move, because a BLAS is now represented by a shared pointer internally. We could, though.

				assert(modelData.mInstances.size() > 0); // Handle the instances. There must at least be one!
				for (size_t i = 0; i < modelData.mInstances.size(); ++i) {
					mGeometryInstances.push_back(
						gvk::context().create_geometry_instance(blas)
							.set_transform_column_major(gvk::to_array( gvk::matrix_from_transforms(modelData.mInstances[i].mTranslation, glm::quat(modelData.mInstances[i].mRotation), modelData.mInstances[i].mScaling) ))
							.set_custom_index(bufferViewIndex)
					);
				}

				// Build BLAS and do not sync at all at this point (passing two empty handlers as parameters):
				//   The idea of this is that multiple BLAS can be built
				//   in parallel, we only have to make sure to synchronize
				//   before we start building the TLAS.
				positionsBuffer.enable_shared_ownership();
				indicesBuffer.enable_shared_ownership();
				mBLASs.back()->build({ avk::vertex_index_buffer_pair{ positionsBuffer, indicesBuffer } }, {},
					avk::sync::with_barriers(
						[posBfr = positionsBuffer, idxBfr = indicesBuffer](avk::command_buffer cb) {
							cb->set_custom_deleter([lPosBfr = std::move(posBfr), lIdxBfr = std::move(idxBfr)](){});
							gvk::context().main_window()->handle_lifetime( avk::owned(cb) );
						}, {}, {})
				);
			}
		}

		// Convert the materials that were gathered above into a GPU-compatible format, and upload into a GPU storage buffer:
		auto [gpuMaterials, imageSamplers] = gvk::convert_for_gpu_usage(
			allMatConfigs, false, true,
			avk::image_usage::general_texture,
			avk::filter_mode::trilinear,
			avk::border_handling_mode::repeat,
			avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler())
		);
		
		mMaterialBuffer = gvk::context().create_buffer(
			avk::memory_usage::host_visible, {},
			avk::storage_buffer_meta::create_from_data(gpuMaterials)
		);
		mMaterialBuffer->fill(
			gpuMaterials.data(), 0,
			avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler())
		);

		mImageSamplers = std::move(imageSamplers);

		auto fif = mainWnd->number_of_frames_in_flight();
		for (decltype(fif) i = 0; i < fif; ++i) {
			// Each TLAS owns every BLAS (this will only work, if the BLASs themselves stay constant, i.e. read access
			auto tlas = gvk::context().create_top_level_acceleration_structure(mGeometryInstances.size(), true);
			// Build the TLAS, ...
			tlas->build(mGeometryInstances, {}, avk::sync::with_barriers(
					gvk::context().main_window()->command_buffer_lifetime_handler(),
					// Sync before building the TLAS:
					[](avk::command_buffer_t& commandBuffer, avk::pipeline_stage destinationStage, std::optional<avk::read_memory_access> readAccess){
						assert(avk::pipeline_stage::acceleration_structure_build == destinationStage);
						assert(!readAccess.has_value() || avk::memory_access::acceleration_structure_read_access == readAccess.value().value());
						// Wait on all the BLAS builds that happened before (and their memory):
						commandBuffer.establish_global_memory_barrier_rw(
							avk::pipeline_stage::acceleration_structure_build, destinationStage,
							avk::memory_access::acceleration_structure_write_access, readAccess
						);
					},
					// Whatever comes after must wait for this TLAS build to finish (and its memory to be made available):
					//   However, that's exactly what the default_handler_after_operation
					//   does, so let's just use that (it is also the default value for
					//   this handler)
					avk::sync::presets::default_handler_after_operation
				)
			);
			mTLAS.push_back(std::move(tlas));
		}
		
		// Create offscreen image views to ray-trace into, one for each frame in flight:
		const auto wdth = gvk::context().main_window()->resolution().x;
		const auto hght = gvk::context().main_window()->resolution().y;
		const auto frmt = gvk::format_from_window_color_buffer(mainWnd);

		for (decltype(fif) i = 0; i < fif; ++i) {
			auto offscreenImage = gvk::context().create_image(wdth, hght, frmt, 1, avk::memory_usage::device, avk::image_usage::general_storage_image);
			offscreenImage->transition_to_layout({}, avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler()));
			mOffscreenImageViews.emplace_back(gvk::context().create_image_view(avk::owned(offscreenImage)));
			assert((mOffscreenImageViews.back()->config().subresourceRange.aspectMask & vk::ImageAspectFlagBits::eColor) == vk::ImageAspectFlagBits::eColor);
		}

		// Create our ray tracing pipeline with the required configuration:
		mPipeline = gvk::context().create_ray_tracing_pipeline_for(
			avk::define_shader_table(
				avk::ray_generation_shader("shaders/rt_09_first.rgen"),
				avk::triangles_hit_group::create_with_rchit_only("shaders/rt_09_first.rchit"),
				avk::triangles_hit_group::create_with_rchit_only("shaders/rt_09_secondary.rchit"),
				avk::miss_shader("shaders/rt_09_first.rmiss.spv"),
				avk::miss_shader("shaders/rt_09_secondary.rmiss.spv")
			),
			gvk::context().get_max_ray_tracing_recursion_depth(),
			// Define push constants and descriptor bindings:
			avk::push_constant_binding_data { avk::shader_type::ray_generation | avk::shader_type::closest_hit, 0, sizeof(push_const_data) },
			avk::descriptor_binding(0, 0, mImageSamplers),
			avk::descriptor_binding(0, 1, mMaterialBuffer),
			avk::descriptor_binding(0, 2, mIndexBufferViews),
			avk::descriptor_binding(0, 3, mTexCoordBufferViews),
			avk::descriptor_binding(1, 0, mOffscreenImageViews[0]->as_storage_image()), // Just take any, this is just to define the layout; could also state it like follows: xk::binding<xk::image_view>(1, 0)
			avk::descriptor_binding(2, 0, mTLAS[0])				 // Just take any, this is just to define the layout; could also state it like follows: xk::binding<xk::top_level_acceleration_structure>(2, 0)
		);

		mPipeline->print_shader_binding_table_groups();

		mPipeline.enable_shared_ownership(); // Make it usable with the updater
		mUpdater.emplace();
		
		// As we are updating our image views if swap chain is resized, it is advised to also cleanup our descriptor cache
		auto descriptorCleanup = [this]() { this->mDescriptorCache.cleanup(); };
		mUpdater->on(gvk::swapchain_resized_event(gvk::context().main_window())).invoke(descriptorCleanup);
		for (auto& oiv : mOffscreenImageViews) {
			oiv.enable_shared_ownership();
			mUpdater->on(gvk::swapchain_resized_event(gvk::context().main_window())).update(oiv);
		}
		mUpdater->on(gvk::swapchain_resized_event(gvk::context().main_window()), gvk::shader_files_changed_event(mPipeline)).update(mPipeline);
		
		// Add the camera to the composition (and let it handle the updates)
		mQuakeCam.set_translation({ 0.0f, 0.0f, 0.0f });
		mQuakeCam.set_perspective_projection(glm::radians(60.0f), gvk::context().main_window()->aspect_ratio(), 0.5f, 100.0f);
		//mQuakeCam.set_orthographic_projection(-5, 5, -5, 5, 0.5, 100);
		gvk::current_composition()->add_element(mQuakeCam);

		auto imguiManager = gvk::current_composition()->element_by_type<gvk::imgui_manager>();
		if (nullptr != imguiManager) {
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

		// Arrow Keys || Page Up/Down Keys => Move the TLAS
		static int64_t updateUntilFrame = -1;
		if (gvk::input().key_down(gvk::key_code::left) || gvk::input().key_down(gvk::key_code::right) || gvk::input().key_down(gvk::key_code::page_down) || gvk::input().key_down(gvk::key_code::page_up) || gvk::input().key_down(gvk::key_code::up) || gvk::input().key_down(gvk::key_code::down)) {
			// Make sure to update all of the in-flight TLASs, otherwise we'll get some geometry jumping:
			updateUntilFrame = gvk::context().main_window()->current_frame() + gvk::context().main_window()->number_of_frames_in_flight() - 1;
		}
		if (gvk::context().main_window()->current_frame() <= updateUntilFrame)
		{
			auto inFlightIndex = gvk::context().main_window()->in_flight_index_for_frame();
			
			auto x = (gvk::input().key_down(gvk::key_code::left)      ? -gvk::time().delta_time() : 0.0f)
					+(gvk::input().key_down(gvk::key_code::right)     ?  gvk::time().delta_time() : 0.0f);
			auto y = (gvk::input().key_down(gvk::key_code::page_down) ? -gvk::time().delta_time() : 0.0f)
					+(gvk::input().key_down(gvk::key_code::page_up)   ?  gvk::time().delta_time() : 0.0f);
			auto z = (gvk::input().key_down(gvk::key_code::up)        ? -gvk::time().delta_time() : 0.0f)
					+(gvk::input().key_down(gvk::key_code::down)      ?  gvk::time().delta_time() : 0.0f);
			auto speed = 1000.0f;
			
			// Change the position of one of the current TLASs BLAS, and update-build the TLAS.
			// The changes do not affect the BLAS, only the instance-data that the TLAS stores to each one of the BLAS.
			//
			// 1. Change every other instance:
			bool evenOdd = true;
			for (auto& geomInst : mGeometryInstances) {
				evenOdd = !evenOdd;
				if (evenOdd) { continue; }
				geomInst.set_transform_column_major(gvk::to_array( glm::translate( gvk::to_mat(geomInst.mTransform), glm::vec3{x, y, z} * speed ) ));
			}
			//
			// 2. Update the TLAS for the current inFlightIndex, copying the changed BLAS-data into an internal buffer:
			mTLAS[inFlightIndex]->update(mGeometryInstances, {}, avk::sync::with_barriers(
				gvk::context().main_window()->command_buffer_lifetime_handler(),
				{}, // Nothing to wait for
				[](avk::command_buffer_t& commandBuffer, avk::pipeline_stage srcStage, std::optional<avk::write_memory_access> srcAccess){
					// We want this update to be as efficient/as tight as possible
					commandBuffer.establish_global_memory_barrier_rw(
						srcStage, avk::pipeline_stage::ray_tracing_shaders, // => ray tracing shaders must wait on the building of the acceleration structure
						srcAccess, avk::memory_access::acceleration_structure_read_access // TLAS-update's memory must be made visible to ray tracing shader's caches (so they can read from)
					);
				}
			));
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
			avk::descriptor_binding(0, 0, mImageSamplers),
			avk::descriptor_binding(0, 1, mMaterialBuffer),
			avk::descriptor_binding(0, 2, mIndexBufferViews),
			avk::descriptor_binding(0, 3, mTexCoordBufferViews),
			avk::descriptor_binding(1, 0, mOffscreenImageViews[inFlightIndex]->as_storage_image()),
			avk::descriptor_binding(2, 0, mTLAS[inFlightIndex])
		}));

		// Set the push constants:
		auto pushConstantsForThisDrawCall = push_const_data { 
			mQuakeCam.global_transformation_matrix(),
			glm::vec4{mLightDir, 0.0f}
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

	avk::buffer mMaterialBuffer;
	std::vector<avk::image_sampler> mImageSamplers;
	std::vector<avk::buffer_view> mIndexBufferViews;
	std::vector<avk::buffer_view> mTexCoordBufferViews;

	glm::vec3 mLightDir = {0.0f, -1.0f, 0.0f};
	
	avk::ray_tracing_pipeline mPipeline;
	avk::graphics_pipeline mGraphicsPipeline;
	gvk::quake_camera mQuakeCam;

}; // ray_tracing_triangle_meshes_app

int main() // <== Starting point ==
{
	try {
		// Create a window and open it
		auto mainWnd = gvk::context().create_window("Real-Time Ray Tracing - Triangle Meshes Example");
		mainWnd->set_resolution({ 1920, 1080 });
		mainWnd->enable_resizing(true);
		mainWnd->set_presentaton_mode(gvk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = gvk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->add_queue_family_ownership(singleQueue);
		mainWnd->set_present_queue(singleQueue);
		
		// Create an instance of our main avk::element which contains all the functionality:
		auto app = ray_tracing_triangle_meshes_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = gvk::imgui_manager(singleQueue);

		// GO:
		gvk::start(
			gvk::application_name("Gears-Vk + Auto-Vk Example: Real-Time Ray Tracing - Triangle Meshes Example"),
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


