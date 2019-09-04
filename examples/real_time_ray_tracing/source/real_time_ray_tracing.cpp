#include <cg_base.hpp>

class real_time_ray_tracing_app : public cgb::cg_element
{
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

		// Merge them all together:

		// The following loop gathers all the vertex and index data PER MATERIAL and constructs the buffers and materials.
		std::vector<cgb::material_config> allMatConfigs;
		for (const auto& pair : distinctMaterialsOrca) {
			// See if we've already encountered this material in the loop above
			auto it = std::find(std::begin(allMatConfigs), std::end(allMatConfigs), pair.first);
			size_t matIndex;
			if (allMatConfigs.end() == it) {
				// Couldn't find => insert new material
				allMatConfigs.push_back(pair.first);
				matIndex = allMatConfigs.size() - 1;
			}
			else {
				// Found the material => use the existing material
				matIndex = std::distance(std::begin(allMatConfigs), it);
			}

			// The data in distinctMaterialsOrca encompasses all of the ORCA scene's models.
			for (const std::tuple<size_t, std::vector<size_t>>& tpl : pair.second) {
				// However, we have to pay attention to the specific model's scene-properties,...
				auto& modelData = orca->model_at_index(std::get<0>(tpl));
				// ... specifically, to its instances:
				// (Generally, we can't combine the vertex data in this case with the vertex
				//  data of other models if we have to draw multiple instances; because in 
				//  the case of multiple instances, only the to-be-instanced geometry must
				//  be accessible independently of the other geometry.
				//  Therefore, in this example, we take the approach of building separate 
				//  buffers for everything which could potentially be instanced.)
				for (size_t i = 0; i < modelData.mInstances.size(); ++i) {
					auto& newElement = mDrawCalls.emplace_back();
					newElement.mMaterialIndex = static_cast<int>(matIndex);
					newElement.mModelMatrix = cgb::matrix_from_transforms(modelData.mInstances[i].mTranslation, glm::quat(modelData.mInstances[i].mRotation), modelData.mInstances[i].mScaling);

					// Get a buffer containing all positions, and one containing all indices for all submeshes with this material
					auto [positionsBuffer, indicesBuffer] = cgb::get_combined_vertex_and_index_buffers_for_selected_meshes({ cgb::make_tuple_model_and_indices(modelData.mLoadedModel, std::get<1>(tpl)) }, 
						[] (auto _Semaphore) {  
							cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
						});
					newElement.mPositionsBuffer = std::move(positionsBuffer);
					newElement.mIndexBuffer = std::move(indicesBuffer);

					// Get a buffer containing all texture coordinates for all submeshes with this material
					newElement.mTexCoordsBuffer = cgb::get_combined_2d_texture_coordinate_buffers_for_selected_meshes({ cgb::make_tuple_model_and_indices(modelData.mLoadedModel, std::get<1>(tpl)) }, 0,
						[] (auto _Semaphore) {  
							cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
						});

					// Get a buffer containing all normals for all submeshes with this material
					newElement.mNormalsBuffer = cgb::get_combined_normal_buffers_for_selected_meshes({ cgb::make_tuple_model_and_indices(modelData.mLoadedModel, std::get<1>(tpl)) }, 
						[] (auto _Semaphore) {  
							cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
						});
				}
			}
		}

		// All the materials have been gathered during the two loops with produced the
		// different vertex- and index-buffers. However, they can potentially share the
		// same material
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
			cgb::binding(0, 0, mImageSamplers),
			cgb::binding(1, 0, mMaterialBuffer)
		);

		// The following is a bit ugly and needs to be abstracted sometime in the future. Sorry for that.
		// Right now it is neccessary to upload the resource descriptors to the GPU (the information about the uniform buffer, in particular).
		// This descriptor set will be used in render(). It is only created once to save memory/to make lifetime management easier.
		mDescriptorSet.reserve(cgb::context().main_window()->number_of_in_flight_frames());
		for (int i = 0; i < cgb::context().main_window()->number_of_in_flight_frames(); ++i) {
			mDescriptorSet.emplace_back(std::make_shared<cgb::descriptor_set>());
			*mDescriptorSet.back() = cgb::descriptor_set::create({ 
				cgb::binding(0, 0, mImageSamplers),
				cgb::binding(1, 0, mMaterialBuffer)
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
		auto cmdbfr = cgb::context().graphics_queue().pool().get_command_buffer(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
		cmdbfr.begin_recording();

		//cmdbfr.begin_render_pass_for_window(cgb::context().main_window());

		// Bind the pipeline
		cmdbfr.handle().bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline->handle());
			

		//// Set the descriptors of the uniform buffer
		//cmdbfr.handle().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mPipeline->layout_handle(), 0, 
		//	mDescriptorSet[cgb::context().main_window()->in_flight_index_for_frame()]->number_of_descriptor_sets(),
		//	mDescriptorSet[cgb::context().main_window()->in_flight_index_for_frame()]->descriptor_sets_addr(), 
		//	0, nullptr);

		//for (auto& drawCall : mDrawCalls) {
		//	// Bind the vertex input buffers in the right order (corresponding to the layout specifiers in the vertex shader)
		//	cmdbfr.handle().bindVertexBuffers(0u, {
		//		drawCall.mPositionsBuffer->buffer_handle(), 
		//		drawCall.mTexCoordsBuffer->buffer_handle(), 
		//		drawCall.mNormalsBuffer->buffer_handle()
		//	}, { 0, 0, 0 });

		// Set the push constants:
		auto pushConstantsForThisDrawCall = transformation_matrices { 
			mQuakeCam.view_matrix()
		};
		cmdbfr.handle().pushConstants(mPipeline->layout_handle(), vk::ShaderStageFlagBits::eRaygenNV, 0, sizeof(pushConstantsForThisDrawCall), &pushConstantsForThisDrawCall);

		//	// Bind and use the index buffer to create the draw call:
		//	vk::IndexType indexType = cgb::to_vk_index_type(drawCall.mIndexBuffer->meta_data().sizeof_one_element());
		//	cmdbfr.handle().bindIndexBuffer(drawCall.mIndexBuffer->buffer_handle(), 0u, indexType);
		//	cmdbfr.handle().drawIndexed(drawCall.mIndexBuffer->meta_data().num_elements(), 1u, 0u, 0u, 0u);
		//}

		//cmdbfr.end_render_pass();
		cmdbfr.end_recording();
		submit_command_buffer_ownership(std::move(cmdbfr));
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

	cgb::storage_buffer mMaterialBuffer;
	std::vector<cgb::image_sampler> mImageSamplers;

	std::vector<std::shared_ptr<cgb::descriptor_set>> mDescriptorSet;

	cgb::ray_tracing_pipeline mPipeline;
	cgb::quake_camera mQuakeCam;

}; // real_time_ray_tracing_app

int main() // <== Starting point ==
{
	try {
		// What's the name of our application
		cgb::settings::gApplicationName = "cg_base::real_time_ray_tracing";

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


