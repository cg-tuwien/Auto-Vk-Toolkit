#include <cg_base.hpp>

class real_time_ray_tracing_app : public cgb::cg_element
{
	struct rt_data
	{
		cgb::vertex_buffer mPositionsBuffer;
		cgb::index_buffer mIndexBuffer;
		int mMaterialIndex;
		glm::mat4 mModelMatrix;
	};

	struct transformation_matrices {
		glm::mat4 mViewMatrix;
	};

public: // v== cgb::cg_element overrides which will be invoked by the framework ==v

	void initialize() override
	{
		mInitTime = std::chrono::high_resolution_clock::now();

		// Load an ORCA scene from file:
		auto orca = cgb::orca_scene_t::load_from_file("assets/sponza.fscene");
		// Get all the different materials from the whole scene:
		auto distinctMaterialsOrca = orca->distinct_material_configs_for_all_models();
		
		// The following loop gathers all the vertex and index data PER MATERIAL and constructs the buffers and materials.
		std::vector<cgb::material_config> allMatConfigs;
		std::vector<rt_data> rtData; 
		rtData.reserve(100); // Due to an internal problem, all the buffers can't properly be moved right now => use `reserve` as a workaround. Sorry, and thanks for your patience. :-S
		for (const auto& pair : distinctMaterialsOrca) {
			auto it = std::find(std::begin(allMatConfigs), std::end(allMatConfigs), pair.first);
			allMatConfigs.push_back(pair.first);
			auto matIndex = allMatConfigs.size() - 1;

			// The data in distinctMaterialsOrca .
			for (const auto& indices : pair.second) {
				// However, we have to pay attention to the specific model's scene-properties,...
				auto& modelData = orca->model_at_index(indices.mModelIndex);
				// ... specifically, to its instances:
				// (Generally, we can't combine the vertex data in this case with the vertex
				//  data of other models if we have to draw multiple instances; because in 
				//  the case of multiple instances, only the to-be-instanced geometry must
				//  be accessible independently of the other geometry.
				//  Therefore, in this example, we take the approach of building separate 
				//  buffers for everything which could potentially be instanced.)
				for (size_t i = 0; i < modelData.mInstances.size(); ++i) {
					auto& newElement = rtData.emplace_back();
					newElement.mMaterialIndex = static_cast<int>(matIndex);
					newElement.mModelMatrix = cgb::matrix_from_transforms(modelData.mInstances[i].mTranslation, glm::quat(modelData.mInstances[i].mRotation), modelData.mInstances[i].mScaling);

					// Get a buffer containing all positions, and one containing all indices for all submeshes with this material
					auto [positionsBuffer, indicesBuffer] = cgb::get_combined_vertex_and_index_buffers_for_selected_meshes({ cgb::make_tuple_model_and_indices(modelData.mLoadedModel, indices.mMeshIndices) });
					newElement.mPositionsBuffer = std::move(positionsBuffer);
					newElement.mPositionsBuffer.enable_shared_ownership();	// This is neccessary currently, until the noexcept-hell has been fixed
					newElement.mIndexBuffer = std::move(indicesBuffer);
					newElement.mIndexBuffer.enable_shared_ownership();		// This is neccessary currently, until the noexcept-hell has been fixed

					//// Get a buffer containing all texture coordinates for all submeshes with this material
					//newElement.mTexCoordsBuffer = cgb::get_combined_2d_texture_coordinate_buffers_for_selected_meshes({ cgb::make_tuple_model_and_indices(modelData.mLoadedModel, std::get<1>(tpl)) }, 0,
					//	[] (auto _Semaphore) {  
					//		cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
					//	});

					//// Get a buffer containing all normals for all submeshes with this material
					//newElement.mNormalsBuffer = cgb::get_combined_normal_buffers_for_selected_meshes({ cgb::make_tuple_model_and_indices(modelData.mLoadedModel, std::get<1>(tpl)) }, 
					//	[] (auto _Semaphore) {  
					//		cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
					//	});
				}
			}

			break;
		}

		// Convert the materials that were gathered above into a GPU-compatible format, and upload into a GPU storage buffer:
		auto [gpuMaterials, imageSamplers] = cgb::convert_for_gpu_usage(allMatConfigs, 
			[](auto _Semaphore) {
				cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore));
			});
		mMaterialBuffer = cgb::create_and_fill(
			cgb::storage_buffer_meta::create_from_data(gpuMaterials),
			cgb::memory_usage::host_coherent,
			gpuMaterials.data()
		);
		mImageSamplers = std::move(imageSamplers);


		mBlas = cgb::bottom_level_acceleration_structure_t::create({ std::make_tuple(std::move(rtData[0].mPositionsBuffer), std::move(rtData[0].mIndexBuffer)) });
		mTlas = cgb::top_level_acceleration_structure_t::create(1);
		{
			auto scratchBuffer = cgb::create(
				cgb::generic_buffer_meta::create_from_size(std::max(mBlas->required_scratch_buffer_build_size(), mTlas->required_scratch_buffer_build_size())),
				cgb::memory_usage::device,
				vk::BufferUsageFlagBits::eRayTracingNV // TODO: This flag is Vulkan-specific, it must be abstracted
			);
			
			//auto commandBuffer = cgb::context().graphics_queue().pool().get_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
			//commandBuffer.begin_recording();

			//auto memoryBarrier = vk::MemoryBarrier()
			//	.setSrcAccessMask(vk::AccessFlagBits::eAccelerationStructureWriteNV | vk::AccessFlagBits::eAccelerationStructureReadNV)
			//	.setDstAccessMask(vk::AccessFlagBits::eAccelerationStructureWriteNV | vk::AccessFlagBits::eAccelerationStructureReadNV);

			//// Build BLAS
			//commandBuffer.handle().buildAccelerationStructureNV(
			//	mBlas->info(),
			//	nullptr, 0,								// no instance data for bottom level AS
			//	VK_FALSE,								// update = false
			//	mBlas->acceleration_structure_handle(), // destination AS
			//	nullptr,								// no source AS
			//	scratchBuffer->buffer_handle(), 0,		// scratch buffer + offset
			//	cgb::context().dynamic_dispatch());

			//// Barrier
			//commandBuffer.handle().pipelineBarrier(
			//	vk::PipelineStageFlagBits::eAccelerationStructureBuildNV,
			//	vk::PipelineStageFlagBits::eAccelerationStructureBuildNV,
			//	vk::DependencyFlags(),
			//	{ memoryBarrier }, 
			//	{}, {});


			//// Temp:
			//std::vector<VkGeometryInstance> geomInstances;
			//{
			//	auto modelMatrixForInstance = glm::transpose(rtData[0].mModelMatrix);
			//	VkGeometryInstance inst;
			//	memcpy(inst.transform, glm::value_ptr(modelMatrixForInstance), sizeof(inst.transform));
			//	inst.instanceId = rtData[0].mMaterialIndex;
			//	inst.mask = 0xff;
			//	inst.instanceOffset = 0;
			//	inst.flags = static_cast<uint32_t>(vk::GeometryInstanceFlagBitsNV::eTriangleCullDisable);
			//	inst.accelerationStructureHandle = mBlas->device_handle();
			//	geomInstances.push_back(inst);
			//}

			//auto geomInstBuffer = cgb::create_and_fill(
			//	cgb::generic_buffer_meta::create_from_data(geomInstances),
			//	cgb::memory_usage::host_coherent,
			//	geomInstances.data(),
			//	nullptr,
			//	vk::BufferUsageFlagBits::eRayTracingNV
			//);

			//// Build TLAS
			//commandBuffer.handle().buildAccelerationStructureNV(
			//	mTlas->info(),
			//	geomInstBuffer->buffer_handle(), 0,	    // buffer containing the instance data (only one)
			//	VK_FALSE,								// update = false
			//	mTlas->acceleration_structure_handle(),	// destination AS
			//	nullptr,								// no source AS
			//	scratchBuffer->buffer_handle(), 0,		// scratch buffer + offset
			//	cgb::context().dynamic_dispatch());

			//// Barrier
			//commandBuffer.handle().pipelineBarrier(
			//	vk::PipelineStageFlagBits::eAccelerationStructureBuildNV,
			//	vk::PipelineStageFlagBits::eRayTracingShaderNV,
			//	vk::DependencyFlags(),
			//	{ memoryBarrier },
			//	{}, {});

			//commandBuffer.end_recording();
			//auto submitInfo = vk::SubmitInfo()
			//	.setCommandBufferCount(1u)
			//	.setPCommandBuffers(commandBuffer.handle_addr());
			//cgb::context().graphics_queue().handle().submit({ submitInfo }, nullptr); 
			//cgb::context().graphics_queue().handle().waitIdle();
		}

		// Create offscreen image views to ray-trace into, one for each frame in flight:
		size_t n = cgb::context().main_window()->number_of_in_flight_frames();
		mOffscreenImageViews.reserve(n);
		for (size_t i = 0; i < n; ++i) {
			mOffscreenImageViews.emplace_back(
				cgb::image_view_t::create(
					cgb::image_t::create(
						cgb::context().main_window()->swap_chain_extent().width, 
						cgb::context().main_window()->swap_chain_extent().height, 
						cgb::context().main_window()->swap_chain_image_format(), 
						false, 1, cgb::memory_usage::device, cgb::image_usage::versatile_image
					)
				)
			);
			cgb::transition_image_layout(
				mOffscreenImageViews.back()->get_image(), 
				mOffscreenImageViews.back()->get_image().format().mFormat, 
				mOffscreenImageViews.back()->get_image().target_layout()); // TODO: This must be abstracted!
			assert((mOffscreenImageViews.back()->config().subresourceRange.aspectMask & vk::ImageAspectFlagBits::eColor) == vk::ImageAspectFlagBits::eColor);
		}

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
			cgb::push_constant_binding_data { cgb::shader_type::ray_generation, 0, sizeof(transformation_matrices) },
			//cgb::binding(0, 0, mImageSamplers),
			//cgb::binding(1, 0, mMaterialBuffer),
			cgb::binding(0, 0, mOffscreenImageViews[0]) // Just take any, this is just to define the layout
			//cgb::binding(0, 1, mTlas)
		);

		//auto swapChainFormat = cgb::context().main_window()->swap_chain_image_format();
		//mGraphicsPipeline = cgb::graphics_pipeline_for(
		//	// Specify which shaders the pipeline consists of:
		//	cgb::vertex_shader("shaders/transform_and_pass_pos_nrm_uv.vert"),
		//	cgb::fragment_shader("shaders/diffuse_shading_fixed_lightsource.frag"),
		//	// The next 3 lines define the format and location of the vertex shader inputs:
		//	// (The dummy values (like glm::vec3) tell the pipeline the format of the respective input)
		//	cgb::vertex_input_location(0, glm::vec3{}).from_buffer_at_binding(0), // <-- corresponds to vertex shader's inPosition
		//	cgb::vertex_input_location(1, glm::vec2{}).from_buffer_at_binding(1), // <-- corresponds to vertex shader's inTexCoord
		//	cgb::vertex_input_location(2, glm::vec3{}).from_buffer_at_binding(2), // <-- corresponds to vertex shader's inNormal
		//	// Some further settings:
		//	cgb::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
		//	cgb::cfg::viewport_depth_scissors_config::from_window(cgb::context().main_window()),
		//	// We'll render to the back buffer, which has a color attachment always, and in our case additionally a depth 
		//	// attachment, which has been configured when creating the window. See main() function!
		//	cgb::attachment::create_color(swapChainFormat),
		//	cgb::attachment::create_depth(),
		//	// The following define additional data which we'll pass to the pipeline:
		//	//   We'll pass two matrices to our vertex shader via push constants:
		//	cgb::push_constant_binding_data { cgb::shader_type::vertex, 0, sizeof(transformation_matrices) },
		//	cgb::binding(0, 0, mImageSamplers),
		//	cgb::binding(1, 0, mMaterialBuffer)
		//);

		// The following is a bit ugly and needs to be abstracted sometime in the future. Sorry for that.
		// Right now it is neccessary to upload the resource descriptors to the GPU (the information about the uniform buffer, in particular).
		// This descriptor set will be used in render(). It is only created once to save memory/to make lifetime management easier.
		mDescriptorSet.reserve(cgb::context().main_window()->number_of_in_flight_frames());
		for (int i = 0; i < cgb::context().main_window()->number_of_in_flight_frames(); ++i) {
			mDescriptorSet.emplace_back(std::make_shared<cgb::descriptor_set>());
			*mDescriptorSet.back() = cgb::descriptor_set::create({ 
				//cgb::binding(0, 0, mImageSamplers),
				//cgb::binding(1, 0, mMaterialBuffer),
				cgb::binding(0, 0, mOffscreenImageViews[i])
				//cgb::binding(2, 0, mTlas)
			});	
		}
		
		// Add the camera to the composition (and let it handle the updates)
		mQuakeCam.set_translation({ 0.0f, 0.0f, 0.0f });
		mQuakeCam.set_perspective_projection(glm::radians(60.0f), cgb::context().main_window()->aspect_ratio(), 0.5f, 100.0f);
		//mQuakeCam.set_orthographic_projection(-5, 5, -5, 5, 0.5, 100);
		cgb::current_composition().add_element(mQuakeCam);
	}

	void render() override
	{
		cgb::context().logical_device().waitIdle();
		static int fuuu = 0; 
		fuuu++;
		LOG_INFO(fmt::format("frame{}", fuuu));
		auto cmdbfr = cgb::context().graphics_queue().pool().get_command_buffer(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
		cmdbfr.begin_recording();

		auto inFlightIndex = cgb::context().main_window()->in_flight_index_for_frame();

		//// Set the descriptors:
		//cmdbfr.begin_render_pass_for_window(cgb::context().main_window());

		//cmdbfr.handle().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mGraphicsPipeline->layout_handle(), 0, 
		//	mDescriptorSet[inFlightIndex]->number_of_descriptor_sets(),
		//	mDescriptorSet[inFlightIndex]->descriptor_sets_addr(), 
		//	0, nullptr);

		//// Bind the pipeline
		//cmdbfr.handle().bindPipeline(vk::PipelineBindPoint::eGraphics, mGraphicsPipeline->handle());
		//
		//// Set the push constants:
		//auto pushConstantsForThisDrawCall = transformation_matrices { 
		//	mQuakeCam.view_matrix()
		//};
		//cmdbfr.handle().pushConstants(mGraphicsPipeline->layout_handle(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(pushConstantsForThisDrawCall), &pushConstantsForThisDrawCall);

		//cmdbfr.handle().draw(0, 0, 0, 0);
		//cmdbfr.end_render_pass();

		// Bind the pipeline
		cmdbfr.handle().bindPipeline(vk::PipelineBindPoint::eRayTracingNV, mPipeline->handle());

		// Set the descriptors:
		cmdbfr.handle().bindDescriptorSets(vk::PipelineBindPoint::eRayTracingNV, mPipeline->layout_handle(), 0, 
			mDescriptorSet[inFlightIndex]->number_of_descriptor_sets(),
			mDescriptorSet[inFlightIndex]->descriptor_sets_addr(), 
			0, nullptr);

		//
		//// Set the push constants:
		//auto pushConstantsForThisDrawCall = transformation_matrices { 
		//	mQuakeCam.view_matrix()
		//};
		//cmdbfr.handle().pushConstants(mPipeline->layout_handle(), vk::ShaderStageFlagBits::eRaygenNV, 0, sizeof(pushConstantsForThisDrawCall), &pushConstantsForThisDrawCall);

		//cmdbfr.handle().traceRaysNV(
		//	mPipeline->shader_binding_table_handle(), 0,
		//	mPipeline->shader_binding_table_handle(), 3 * mPipeline->table_entry_size(), mPipeline->table_entry_size(),
		//	mPipeline->shader_binding_table_handle(), 1 * mPipeline->table_entry_size(), mPipeline->table_entry_size(),
		//	nullptr, 0, 0,
		//	cgb::context().main_window()->swap_chain_extent().width, cgb::context().main_window()->swap_chain_extent().height, 1,
		//	cgb::context().dynamic_dispatch());

		cgb::context().graphics_queue().handle().waitIdle();
		
		cmdbfr.set_image_barrier(
			cgb::create_image_barrier(
				cgb::context().main_window()->swap_chain_images()[inFlightIndex], 
				cgb::context().main_window()->swap_chain_image_format().mFormat, 
				vk::AccessFlags(), vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal)
		);
		cmdbfr.set_image_barrier(
			cgb::create_image_barrier(
				mOffscreenImageViews[inFlightIndex]->get_image().image_handle(),
				mOffscreenImageViews[inFlightIndex]->get_image().format().mFormat,
				vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eTransferRead, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal)
		);

		cmdbfr.copy_image(mOffscreenImageViews[inFlightIndex]->get_image(), cgb::context().main_window()->swap_chain_images()[inFlightIndex]);

		cmdbfr.set_image_barrier(
			cgb::create_image_barrier(
				cgb::context().main_window()->swap_chain_images()[inFlightIndex],
				cgb::context().main_window()->swap_chain_image_format().mFormat,
				vk::AccessFlagBits::eTransferWrite, vk::AccessFlags(), vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR
			)
		);

		cgb::context().graphics_queue().handle().waitIdle();

		cgb::context().logical_device().waitIdle();

		//cmdbfr.end_render_pass();
		cmdbfr.end_recording();
		submit_command_buffer_ownership(std::move(cmdbfr));
		cgb::context().logical_device().waitIdle();

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
	}

	void finalize() override
	{
		cgb::context().logical_device().waitIdle();
	}

private: // v== Member variables ==v
	std::chrono::high_resolution_clock::time_point mInitTime;

	cgb::bottom_level_acceleration_structure mBlas;
	cgb::top_level_acceleration_structure mTlas;

	std::vector<cgb::image_view> mOffscreenImageViews;

	cgb::storage_buffer mMaterialBuffer;
	std::vector<cgb::image_sampler> mImageSamplers;

	std::vector<std::shared_ptr<cgb::descriptor_set>> mDescriptorSet;

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

		// Create a window and open it
		auto mainWnd = cgb::context().create_window("cg_base: Real-Time Ray Tracing Example");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->set_presentaton_mode(cgb::presentation_mode::vsync);
		mainWnd->set_additional_back_buffer_attachments({ cgb::attachment::create_depth(cgb::image_format::default_depth_format()) });
		mainWnd->open(); 

		// Create an instance of vertex_buffers_app which, in this case,
		// contains the entire functionality of our application. 
		auto element = real_time_ray_tracing_app();

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


