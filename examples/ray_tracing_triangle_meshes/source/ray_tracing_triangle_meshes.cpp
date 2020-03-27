#include <cg_base.hpp>

class ray_tracing_triangle_meshes_app : public cgb::cg_element
{
	struct push_const_data {
		glm::mat4 mViewMatrix;
		glm::vec4 mLightDirection;
	};

public: // v== cgb::cg_element overrides which will be invoked by the framework ==v

	void initialize() override
	{
		mInitTime = std::chrono::high_resolution_clock::now();
		mLightDir = { 0.8f, 1.0f, 0.0f, 0.0f };

		// Load an ORCA scene from file:
		auto orca = cgb::orca_scene_t::load_from_file("assets/sponza.fscene");
		// Get all the different materials from the whole scene:
		auto distinctMaterialsOrca = orca->distinct_material_configs_for_all_models();
		
		// The following loop gathers all the vertex and index data PER MATERIAL and constructs the buffers and materials.
		std::vector<cgb::material_config> allMatConfigs;
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
				auto [positionsBuffer, indicesBuffer] = cgb::get_combined_vertex_and_index_buffers_for_selected_meshes(
					{ cgb::make_tuple_model_and_indices(modelData.mLoadedModel, indices.mMeshIndices) },
					cgb::sync::with_barriers_on_current_frame()
				);
				
				// Get a buffer containing all texture coordinates for all submeshes with this material
				auto bufferViewIndex = static_cast<uint32_t>(mTexCoordBufferViews.size());
				auto texCoordsData = cgb::get_combined_2d_texture_coordinates_for_selected_meshes({ cgb::make_tuple_model_and_indices(modelData.mLoadedModel, indices.mMeshIndices) }, 0);
				auto texCoordsTexelBuffer = cgb::create_and_fill(
					cgb::uniform_texel_buffer_meta::create_from_data(texCoordsData)
						.describe_only_member(texCoordsData[0]),
					cgb::memory_usage::device,
					texCoordsData.data(),
					cgb::sync::with_barriers_on_current_frame()
				);
				mTexCoordBufferViews.push_back( cgb::buffer_view_t::create(std::move(texCoordsTexelBuffer)) );

				// The following call is quite redundant => TODO: optimize!
				auto [positionsData, indicesData] = cgb::get_combined_vertices_and_indices_for_selected_meshes({ cgb::make_tuple_model_and_indices(modelData.mLoadedModel, indices.mMeshIndices) });
				auto indexTexelBuffer = cgb::create_and_fill(
					cgb::uniform_texel_buffer_meta::create_from_data(indicesData)
						.set_format<glm::uvec3>(), // Combine 3 consecutive elements to one unit
					cgb::memory_usage::device,
					indicesData.data(),
					cgb::sync::with_barriers_on_current_frame()
				);
				mIndexBufferViews.push_back( cgb::buffer_view_t::create(std::move(indexTexelBuffer)) );

				// Create one bottom level acceleration structure per model
				auto blas = cgb::bottom_level_acceleration_structure_t::create(std::move(positionsBuffer), std::move(indicesBuffer));
				// Enable shared ownership because we'll have one TLAS per frame in flight, each one referencing the SAME BLASs
				// (But that also means that we may not modify the BLASs. They must stay the same, otherwise concurrent access will fail.)
				blas.enable_shared_ownership();
				mBLASs.push_back(blas); // No need to move, because a BLAS is now represented by a shared pointer internally. We could, though.

				assert(modelData.mInstances.size() > 0); // Handle the instances. There must at least be one!
				for (size_t i = 0; i < modelData.mInstances.size(); ++i) {
					mGeometryInstances.push_back(
						cgb::geometry_instance::create(blas)
							.set_transform(cgb::matrix_from_transforms(modelData.mInstances[i].mTranslation, glm::quat(modelData.mInstances[i].mRotation), modelData.mInstances[i].mScaling))
							.set_custom_index(bufferViewIndex)
					);
				}

				// Build BLAS and do not sync at all at this point (passing two empty handlers as parameters):
				//   The idea of this is that multiple BLAS can be built
				//   in parallel, we only have to make sure to synchronize
				//   before we start building the TLAS.
				mBLASs.back()->build(cgb::sync::with_barriers_on_current_frame({}, {}));
			}
		}

		// Convert the materials that were gathered above into a GPU-compatible format, and upload into a GPU storage buffer:
		auto [gpuMaterials, imageSamplers] = cgb::convert_for_gpu_usage(
			allMatConfigs,
			cgb::image_usage::read_only_sampled_image,
			cgb::filter_mode::bilinear,
			cgb::border_handling_mode::repeat,
			cgb::sync::with_barriers_on_current_frame());
		
		mMaterialBuffer = cgb::create_and_fill(
			cgb::storage_buffer_meta::create_from_data(gpuMaterials),
			cgb::memory_usage::host_coherent,
			gpuMaterials.data(),
			cgb::sync::with_barriers_on_current_frame()
		);

		mImageSamplers = std::move(imageSamplers);

		auto fif = cgb::context().main_window()->number_of_in_flight_frames();
		cgb::invoke_for_all_in_flight_frames(cgb::context().main_window(), [&](auto inFlightIndex) {
			// Each TLAS owns every BLAS (this will only work, if the BLASs themselves stay constant, i.e. read access
			auto tlas = cgb::top_level_acceleration_structure_t::create(mGeometryInstances.size());
			// Build the TLAS, ...
			tlas->build(mGeometryInstances, cgb::sync::with_barriers_on_current_frame(
					// Sync before building the TLAS:
					[](cgb::command_buffer_t& commandBuffer, cgb::pipeline_stage destinationStage, std::optional<cgb::read_memory_access> readAccess){
						assert(cgb::pipeline_stage::acceleration_structure_build == destinationStage);
						assert(!readAccess.has_value() || cgb::memory_access::acceleration_structure_read_access == readAccess.value().value());
						// Wait on all the BLAS builds that happened before (and their memory):
						commandBuffer.establish_global_memory_barrier(
							cgb::pipeline_stage::acceleration_structure_build, destinationStage,
							cgb::memory_access::acceleration_structure_write_access, readAccess
						);
					},
					// Whatever comes after must wait for this TLAS build to finish (and its memory to be made available):
					//   However, that's exactly what the default_handler_after_operation
					//   does, so let's just use that (it is also the default value for
					//   this handler)
					cgb::sync::default_handler_after_operation
				)
			);
			mTLAS.push_back(std::move(tlas));
		});
		
		// Create offscreen image views to ray-trace into, one for each frame in flight:
		size_t n = cgb::context().main_window()->number_of_in_flight_frames();
		const auto wdth = cgb::context().main_window()->resolution().x;
		const auto hght = cgb::context().main_window()->resolution().y;
		const auto frmt = cgb::image_format::from_window_color_buffer(cgb::context().main_window());
		cgb::invoke_for_all_in_flight_frames(cgb::context().main_window(), [&](auto inFlightIndex){
			mOffscreenImageViews.emplace_back(
				cgb::image_view_t::create(
					cgb::image_t::create(wdth, hght, frmt, false, 1, cgb::memory_usage::device, cgb::image_usage::versatile_image)
				)
			);
			mOffscreenImageViews.back()->get_image().transition_to_layout({}, cgb::sync::with_barriers_on_current_frame());
			assert((mOffscreenImageViews.back()->config().subresourceRange.aspectMask & vk::ImageAspectFlagBits::eColor) == vk::ImageAspectFlagBits::eColor);
		});

		// Create our ray tracing pipeline with the required configuration:
		mPipeline = cgb::ray_tracing_pipeline_for(
			cgb::define_shader_table(
				cgb::ray_generation_shader("shaders/rt_09_first.rgen"),
				cgb::triangles_hit_group::create_with_rchit_only("shaders/rt_09_first.rchit"),
				cgb::triangles_hit_group::create_with_rchit_only("shaders/rt_09_secondary.rchit"),
				cgb::miss_shader("shaders/rt_09_first.rmiss.spv"),
				cgb::miss_shader("shaders/rt_09_secondary.rmiss.spv")
			),
			cgb::max_recursion_depth::set_to_max(),
			// Define push constants and descriptor bindings:
			cgb::push_constant_binding_data { cgb::shader_type::ray_generation | cgb::shader_type::closest_hit, 0, sizeof(push_const_data) },
			cgb::binding(0, 0, mImageSamplers),
			cgb::binding(0, 1, mMaterialBuffer),
			cgb::binding(0, 2, mIndexBufferViews),
			cgb::binding(0, 3, mTexCoordBufferViews),
			cgb::binding(1, 0, mOffscreenImageViews[0]), // Just take any, this is just to define the layout
			cgb::binding(2, 0, mTLAS[0])				 // Just take any, this is just to define the layout
		);

		//// The following is a bit ugly and needs to be abstracted sometime in the future. Sorry for that.
		//// Right now it is neccessary to upload the resource descriptors to the GPU (the information about the uniform buffer, in particular).
		//// This descriptor set will be used in render(). It is only created once to save memory/to make lifetime management easier.
		//for (int i = 0; i < cgb::context().main_window()->number_of_in_flight_frames(); ++i) {
		//	mDescriptorSet.emplace_back(std::make_shared<cgb::descriptor_set>());
		//	*mDescriptorSet.back() = cgb::descriptor_set::create({ 
		//		cgb::binding(0, 0, mImageSamplers),
		//		cgb::binding(0, 1, mMaterialBuffer),
		//		cgb::binding(0, 2, mIndexBufferViews),
		//		cgb::binding(0, 3, mTexCoordBufferViews),
		//		cgb::binding(1, 0, mOffscreenImageViews[i]),
		//		cgb::binding(2, 0, mTLAS[i])
		//	});	
		//}
		
		// Add the camera to the composition (and let it handle the updates)
		mQuakeCam.set_translation({ 0.0f, 0.0f, 0.0f });
		mQuakeCam.set_perspective_projection(glm::radians(60.0f), cgb::context().main_window()->aspect_ratio(), 0.5f, 100.0f);
		//mQuakeCam.set_orthographic_projection(-5, 5, -5, 5, 0.5, 100);
		cgb::current_composition().add_element(mQuakeCam);
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
		if (cgb::input().key_down(cgb::key_code::left) || cgb::input().key_down(cgb::key_code::right) || cgb::input().key_down(cgb::key_code::page_down) || cgb::input().key_down(cgb::key_code::page_up) || cgb::input().key_down(cgb::key_code::up) || cgb::input().key_down(cgb::key_code::down)) {
			// Make sure to update all of the in-flight TLASs, otherwise we'll get some geometry jumping:
			updateUntilFrame = cgb::context().main_window()->current_frame() + cgb::context().main_window()->number_of_in_flight_frames() - 1;
		}
		if (cgb::context().main_window()->current_frame() <= updateUntilFrame)
		{
			auto inFlightIndex = cgb::context().main_window()->in_flight_index_for_frame();
			
			auto x = (cgb::input().key_down(cgb::key_code::left)      ? -cgb::time().delta_time() : 0.0f)
					+(cgb::input().key_down(cgb::key_code::right)     ?  cgb::time().delta_time() : 0.0f);
			auto y = (cgb::input().key_down(cgb::key_code::page_down) ? -cgb::time().delta_time() : 0.0f)
					+(cgb::input().key_down(cgb::key_code::page_up)   ?  cgb::time().delta_time() : 0.0f);
			auto z = (cgb::input().key_down(cgb::key_code::up)        ? -cgb::time().delta_time() : 0.0f)
					+(cgb::input().key_down(cgb::key_code::down)      ?  cgb::time().delta_time() : 0.0f);
			auto speed = 1000.0f;
			
			// Change the position of one of the current TLASs BLAS, and update-build the TLAS.
			// The changes do not affect the BLAS, only the instance-data that the TLAS stores to each one of the BLAS.
			//
			// 1. Change every other instance:
			bool evenOdd = true;
			for (auto& geomInst : mGeometryInstances) {
				evenOdd = !evenOdd;
				if (evenOdd) { continue; }
				geomInst.set_transform(glm::translate(geomInst.mTransform, glm::vec3{x, y, z} * speed));
			}
			//
			// 2. Update the TLAS for the current inFlightIndex, copying the changed BLAS-data into an internal buffer:
			mTLAS[inFlightIndex]->update(mGeometryInstances, cgb::sync::with_barriers_on_current_frame(
				{}, // Nothing to wait for
				[](cgb::command_buffer_t& commandBuffer, cgb::pipeline_stage srcStage, std::optional<cgb::write_memory_access> srcAccess){
					// We want this update to be as efficient/as tight as possible
					commandBuffer.establish_global_memory_barrier(
						srcStage, cgb::pipeline_stage::ray_tracing_shaders, // => ray tracing shaders must wait on the building of the acceleration structure
						srcAccess, cgb::memory_access::acceleration_structure_read_access // TLAS-update's memory must be made visible to ray tracing shader's caches (so they can read from)
					);
				}
			));
		}

		if (cgb::input().key_pressed(cgb::key_code::space)) {
			// Print the current camera position
			auto pos = mQuakeCam.translation();
			LOG_INFO(fmt::format("Current camera position: {}", cgb::to_string(pos)));
		}
		if (cgb::input().key_pressed(cgb::key_code::escape)) {
			// Stop the current composition:
			cgb::current_composition().stop();
		}
		if (cgb::input().key_pressed(cgb::key_code::tab)) {
			if (mQuakeCam.is_enabled()) {
				mQuakeCam.disable();
			}
			else {
				mQuakeCam.enable();
			}
		}

		if (cgb::input().key_down(cgb::key_code::j)) {
			mLightDir = glm::vec4( glm::mat3(glm::rotate( cgb::time().delta_time(), glm::vec3{1.0f, 0.f, 0.f})) * glm::vec3(mLightDir), 0.0f);
		}
		if (cgb::input().key_down(cgb::key_code::l)) {
			mLightDir = glm::vec4( glm::mat3(glm::rotate(-cgb::time().delta_time(), glm::vec3{1.0f, 0.f, 0.f})) * glm::vec3(mLightDir), 0.0f);
		}
		if (cgb::input().key_down(cgb::key_code::i)) {
			mLightDir = glm::vec4( glm::mat3(glm::rotate( cgb::time().delta_time(), glm::vec3{0.0f, 0.f, 1.f})) * glm::vec3(mLightDir), 0.0f);
		}
		if (cgb::input().key_down(cgb::key_code::k)) {
			mLightDir = glm::vec4( glm::mat3(glm::rotate(-cgb::time().delta_time(), glm::vec3{0.0f, 0.f, 1.f})) * glm::vec3(mLightDir), 0.0f);
		}
		if (cgb::input().key_down(cgb::key_code::u)) {
			mLightDir = glm::vec4( glm::mat3(glm::rotate( cgb::time().delta_time(), glm::vec3{0.0f, 1.f, 0.f})) * glm::vec3(mLightDir), 0.0f);
		}
		if (cgb::input().key_down(cgb::key_code::o)) {
			mLightDir = glm::vec4( glm::mat3(glm::rotate(-cgb::time().delta_time(), glm::vec3{0.0f, 1.f, 0.f})) * glm::vec3(mLightDir), 0.0f);
		}
	}

	void render() override
	{
		auto inFlightIndex = cgb::context().main_window()->in_flight_index_for_frame();
		
		auto cmdbfr = cgb::context().graphics_queue().create_single_use_command_buffer();
		cmdbfr->begin_recording();
		cmdbfr->bind_pipeline(mPipeline);
		cmdbfr->bind_descriptors(mPipeline->layout(), { 
				cgb::binding(0, 0, mImageSamplers),
				cgb::binding(0, 1, mMaterialBuffer),
				cgb::binding(0, 2, mIndexBufferViews),
				cgb::binding(0, 3, mTexCoordBufferViews),
				cgb::binding(1, 0, mOffscreenImageViews[inFlightIndex]),
				cgb::binding(2, 0, mTLAS[inFlightIndex])
			}
		);

		// Set the push constants:
		auto pushConstantsForThisDrawCall = push_const_data { 
			mQuakeCam.view_matrix(),
			mLightDir
		};
		cmdbfr->handle().pushConstants(mPipeline->layout_handle(), vk::ShaderStageFlagBits::eRaygenNV | vk::ShaderStageFlagBits::eClosestHitNV, 0, sizeof(pushConstantsForThisDrawCall), &pushConstantsForThisDrawCall);

		// TRACE. THA. RAYZ.
		cmdbfr->handle().traceRaysNV(
			mPipeline->shader_binding_table_handle(), 0,
			mPipeline->shader_binding_table_handle(), 3 * mPipeline->table_entry_size(), mPipeline->table_entry_size(),
			mPipeline->shader_binding_table_handle(), 1 * mPipeline->table_entry_size(), mPipeline->table_entry_size(),
			nullptr, 0, 0,
			cgb::context().main_window()->swap_chain_extent().width, cgb::context().main_window()->swap_chain_extent().height, 1,
			cgb::context().dynamic_dispatch());

		cmdbfr->end_recording();
		submit_command_buffer_ownership(std::move(cmdbfr));
		present_image(mOffscreenImageViews[inFlightIndex]->get_image());
	}

private: // v== Member variables ==v
	std::chrono::high_resolution_clock::time_point mInitTime;

	// Multiple BLAS, concurrently used by all the (three?) TLASs:
	std::vector<cgb::bottom_level_acceleration_structure> mBLASs;
	// Geometry instance data which store the instance data per BLAS
	std::vector<cgb::geometry_instance> mGeometryInstances;
	// Multiple TLAS, one for each frame in flight:
	std::vector<cgb::top_level_acceleration_structure> mTLAS;

	std::vector<cgb::image_view> mOffscreenImageViews;

	cgb::storage_buffer mMaterialBuffer;
	std::vector<cgb::image_sampler> mImageSamplers;
	std::vector<cgb::buffer_view> mIndexBufferViews;
	std::vector<cgb::buffer_view> mTexCoordBufferViews;

	std::vector<std::shared_ptr<cgb::descriptor_set>> mDescriptorSet;

	glm::vec4 mLightDir;
	
	cgb::ray_tracing_pipeline mPipeline;
	cgb::graphics_pipeline mGraphicsPipeline;
	cgb::quake_camera mQuakeCam;

}; // real_time_ray_tracing_app

int main() // <== Starting point ==
{
	try {
		// What's the name of our application
		cgb::settings::gApplicationName = "cg_base::real_time_ray_tracing";
		cgb::settings::gQueueSelectionPreference = cgb::device_queue_selection_strategy::prefer_everything_on_single_queue;
		cgb::settings::gRequiredDeviceExtensions.push_back(VK_NV_RAY_TRACING_EXTENSION_NAME);
		cgb::settings::gRequiredDeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
		cgb::settings::gLoadImagesInSrgbFormatByDefault = true;

		// Create a window and open it
		auto mainWnd = cgb::context().create_window("cg_base: Real-Time Ray Tracing - Triangle Meshes Example");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->set_presentaton_mode(cgb::presentation_mode::vsync);
		mainWnd->set_additional_back_buffer_attachments({ cgb::attachment::create_depth(cgb::image_format::default_depth_format()) });
		mainWnd->open(); 

		// Create an instance of ray_tracing_triangle_meshes_app which, in this case,
		// contains the entire functionality of our application. 
		auto element = ray_tracing_triangle_meshes_app();

		// Create a composition of:
		//  - a timer
		//  - an executor
		//  - a behavior
		// ...
		auto hello = cgb::composition<cgb::varying_update_timer, cgb::sequential_executor>({
				&element
			});

		// ... and start that composition!
		hello.start();
	}
	catch (std::runtime_error& re)
	{
		LOG_ERROR_EM(re.what());
		//throw re;
	}
}


