#include <cg_base.hpp>

class orca_loader_app : public cgb::cg_element
{
	struct data_for_draw_call
	{
		cgb::vertex_buffer mPositionsBuffer;
		cgb::vertex_buffer mTexCoordsBuffer;
		cgb::vertex_buffer mNormalsBuffer;
		cgb::index_buffer mIndexBuffer;
		int mMaterialIndex;
		glm::mat4 mModelMatrix;
	};

	struct transformation_matrices {
		glm::mat4 mModelMatrix;
		glm::mat4 mProjViewMatrix;
		int mMaterialIndex;
	};

public: // v== cgb::cg_element overrides which will be invoked by the framework ==v

	cgb::model sponza;
	cgb::orca_scene orca;

	void initialize() override
	{
		mInitTime = std::chrono::high_resolution_clock::now();

		// Load a model from file:
		sponza = cgb::model_t::load_from_file("S:/ORCA/SunTemple/SunTemple.fbx", aiProcess_Triangulate | aiProcess_PreTransformVertices);
		// Get all the different materials of the model:
		auto distinctMaterialsSponza = sponza->distinct_material_configs();

		//// Load an ORCA scene from file:
		//orca = cgb::orca_scene_t::load_from_file("S:/ORCA/SunTemple/SunTemple.fscene");
		//// Get all the different materials from the whole scene:
		//auto distinctMaterialsOrca = orca->distinct_material_configs_for_all_models();

		// Merge them all together:

		mDrawCalls.reserve(distinctMaterialsSponza.size() /*+ distinctMaterialsOrca.size()*/); // Due to an internal error, all the buffers can't properly be moved right now => use `reserve` as a workaround. Sorry, and thanks for your patience. :-S
		
		// The following loop gathers all the vertex and index data PER MATERIAL and constructs the buffers and materials.
		// Later, we'll use ONE draw call PER MATERIAL to draw the whole scene.
		std::vector<cgb::material_config> allMatCofigs;
		for (const auto& pair : distinctMaterialsSponza) {
			allMatCofigs.push_back(pair.first);

			auto& newElement = mDrawCalls.emplace_back();
			newElement.mMaterialIndex = static_cast<int>(allMatCofigs.size() - 1);
			newElement.mModelMatrix = glm::scale(glm::vec3(0.01f));
			
			// Compared to the "model_loader" example, we are taking a mor optimistic appproach here.
			// By not using `cgb::append_indices_and_vertex_data` directly, we have no guarantee that
			// all vertex arrays are of the same length. 
			// Instead, here we use the (possibly more convenient) `cgb::get_combined*` functions and
			// just optimistically assume that positions, texture coordinates, and normals are all of
			// the same length.

			// Get a buffer containing all positions, and one containing all indices for all submeshes with this material
			auto [positionsBuffer, indicesBuffer] = cgb::get_combined_vertex_and_index_buffers_for_selected_meshes({ cgb::make_tuple_model_and_indices(sponza, pair.second) }, 
				[] (auto _Semaphore) {  
					cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
				});
			newElement.mPositionsBuffer = std::move(positionsBuffer);
			newElement.mIndexBuffer = std::move(indicesBuffer);

			// Get a buffer containing all texture coordinates for all submeshes with this material
			newElement.mTexCoordsBuffer = cgb::get_combined_2d_texture_coordinate_buffers_for_selected_meshes({ cgb::make_tuple_model_and_indices(sponza, pair.second) }, 0,
				[] (auto _Semaphore) {  
					cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
				});

			// Get a buffer containing all normals for all submeshes with this material
			newElement.mNormalsBuffer = cgb::get_combined_normal_buffers_for_selected_meshes({ cgb::make_tuple_model_and_indices(sponza, pair.second) }, 
				[] (auto _Semaphore) {  
					cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
				});
		}
		// Same for the ORCA scene:
		//for (const auto& pair : distinctMaterialsOrca) {
		//	allMatCofigs.push_back(pair.first);
		//	
		//	for (const std::tuple<size_t, std::vector<size_t>>& tpl : pair.second) {

		//		auto& modelData = orca->model_at_index(std::get<0>(tpl));

		//		for (size_t i = 0; i < modelData.mInstances.size(); ++i) {
		//			auto& newElement = mDrawCalls.emplace_back();
		//			newElement.mMaterialIndex = static_cast<int>(allMatCofigs.size() - 1);
		//			newElement.mModelMatrix = cgb::matrix_from_transforms(modelData.mInstances[i].mTranslation, glm::quat(modelData.mInstances[i].mTranslation), modelData.mInstances[i].mScaling);

		//			// Get a buffer containing all positions, and one containing all indices for all submeshes with this material
		//			auto [positionsBuffer, indicesBuffer] = cgb::get_combined_vertex_and_index_buffers_for_selected_meshes({ cgb::make_tuple_model_and_indices(modelData.mLoadedModel, std::get<1>(tpl)) }, 
		//				[] (auto _Semaphore) {  
		//					cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
		//				});
		//			newElement.mPositionsBuffer = std::move(positionsBuffer);
		//			newElement.mIndexBuffer = std::move(indicesBuffer);

		//			// Get a buffer containing all texture coordinates for all submeshes with this material
		//			newElement.mTexCoordsBuffer = cgb::get_combined_2d_texture_coordinate_buffers_for_selected_meshes({ cgb::make_tuple_model_and_indices(modelData.mLoadedModel, std::get<1>(tpl)) }, 0,
		//				[] (auto _Semaphore) {  
		//					cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
		//				});

		//			// Get a buffer containing all normals for all submeshes with this material
		//			newElement.mNormalsBuffer = cgb::get_combined_normal_buffers_for_selected_meshes({ cgb::make_tuple_model_and_indices(modelData.mLoadedModel, std::get<1>(tpl)) }, 
		//				[] (auto _Semaphore) {  
		//					cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
		//				});
		//		}
		//	}
		//}

		auto [gpuMaterials, imageSamplers] = cgb::convert_for_gpu_usage(allMatCofigs, 
			[](auto _Semaphore) {
				cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore));
			});
		mGpuMaterialData = std::move(gpuMaterials);
		mImageSamplers = std::move(imageSamplers);

		mMaterialBuffer.reserve(cgb::context().main_window()->number_of_in_flight_frames());
		for (int i = 0; i < cgb::context().main_window()->number_of_in_flight_frames(); ++i) {
			mMaterialBuffer.emplace_back(cgb::create_and_fill(
				cgb::storage_buffer_meta::create_from_data(mGpuMaterialData),
				cgb::memory_usage::host_coherent,
				&mGpuMaterialData[0]
			));			
		}

		auto swapChainFormat = cgb::context().main_window()->swap_chain_image_format();
		// Create our rasterization graphics pipeline with the required configuration:
		mPipeline = cgb::graphics_pipeline_for(
			// Specify which shaders the pipeline consists of:
			cgb::vertex_shader("shaders/transform_and_pass_pos_nrm_uv.vert"),
			cgb::fragment_shader("shaders/diffuse_shading_fixed_lightsource.frag"),
			// The next 3 lines define the format and location of the vertex shader inputs:
			// (The dummy values (like glm::vec3) tell the pipeline the format of the respective input)
			cgb::vertex_input_location(0, glm::vec3{}).from_buffer_at_binding(0), // <-- corresponds to vertex shader's inPosition
			cgb::vertex_input_location(1, glm::vec2{}).from_buffer_at_binding(1), // <-- corresponds to vertex shader's inTexCoord
			cgb::vertex_input_location(2, glm::vec3{}).from_buffer_at_binding(2), // <-- corresponds to vertex shader's inNormal
			// Some further settings:
			cgb::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			cgb::cfg::viewport_depth_scissors_config::from_window(cgb::context().main_window()),
			// We'll render to the back buffer, which has a color attachment always, and in our case additionally a depth 
			// attachment, which has been configured when creating the window. See main() function!
			cgb::attachment::create_color(swapChainFormat),
			cgb::attachment::create_depth(),
			// The following define additional data which we'll pass to the pipeline:
			//   We'll pass two matrices to our vertex shader via push constants:
			cgb::push_constant_binding_data { cgb::shader_type::vertex, 0, sizeof(transformation_matrices) },
			cgb::binding(0, 0, mImageSamplers),
			cgb::binding(1, 0, mMaterialBuffer[0]) // Just take any buffer, this is just for the layout
		);

		// The following is a bit ugly and needs to be abstracted sometime in the future. Sorry for that.
		// Right now it is neccessary to upload the resource descriptors to the GPU (the information about the uniform buffer, in particular).
		// This descriptor set will be used in render(). It is only created once to save memory/to make lifetime management easier.
		mDescriptorSet.reserve(cgb::context().main_window()->number_of_in_flight_frames());
		for (int i = 0; i < cgb::context().main_window()->number_of_in_flight_frames(); ++i) {
			mDescriptorSet.emplace_back(std::make_shared<cgb::descriptor_set>());
			*mDescriptorSet.back() = std::move(cgb::descriptor_set::create({ 
				cgb::binding(0, 0, mImageSamplers),
				cgb::binding(1, 0, mMaterialBuffer[i])
			}));	
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

		cmdbfr.begin_render_pass_for_window(cgb::context().main_window());

		// Bind the pipeline
		cmdbfr.handle().bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline->handle());
			

		// Set the descriptors of the uniform buffer
		cmdbfr.handle().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mPipeline->layout_handle(), 0, 
			mDescriptorSet[cgb::context().main_window()->in_flight_index_for_frame()]->number_of_descriptor_sets(),
			mDescriptorSet[cgb::context().main_window()->in_flight_index_for_frame()]->descriptor_sets_addr(), 
			0, nullptr);

		for (auto& drawCall : mDrawCalls) {
			// Bind the vertex input buffers in the right order (corresponding to the layout specifiers in the vertex shader)
			cmdbfr.handle().bindVertexBuffers(0u, {
				drawCall.mPositionsBuffer->buffer_handle(), 
				drawCall.mTexCoordsBuffer->buffer_handle(), 
				drawCall.mNormalsBuffer->buffer_handle()
			}, { 0, 0, 0 });

			// Set the push constants:
			auto pushConstantsForThisDrawCall = transformation_matrices { 
				drawCall.mModelMatrix,									// <-- mModelMatrix
				mQuakeCam.projection_matrix() * mQuakeCam.view_matrix(),// <-- mProjViewMatrix
				drawCall.mMaterialIndex
			};
			cmdbfr.handle().pushConstants(mPipeline->layout_handle(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(pushConstantsForThisDrawCall), &pushConstantsForThisDrawCall);

			// Bind and use the index buffer to create the draw call:
			vk::IndexType indexType = cgb::to_vk_index_type(drawCall.mIndexBuffer->meta_data().sizeof_one_element());
			cmdbfr.handle().bindIndexBuffer(drawCall.mIndexBuffer->buffer_handle(), 0u, indexType);
			cmdbfr.handle().drawIndexed(drawCall.mIndexBuffer->meta_data().num_elements(), 1u, 0u, 0u, 0u);
		}

		cmdbfr.end_render_pass();
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

		if (cgb::input().key_pressed(cgb::key_code::h)) {
			// Log a message:
			LOG_INFO_EM("Hello cg_base!");
		}
		if (cgb::input().key_pressed(cgb::key_code::c)) {
			// Center the cursor:
			auto resolution = cgb::context().main_window()->resolution();
			cgb::context().main_window()->set_cursor_pos({ resolution[0] / 2.0, resolution[1] / 2.0 });
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

	std::vector<cgb::material_gpu_data> mGpuMaterialData;
	std::vector<cgb::image_sampler> mImageSamplers;

	std::vector<data_for_draw_call> mDrawCalls;
	std::vector<cgb::storage_buffer> mMaterialBuffer;
	std::vector<std::shared_ptr<cgb::descriptor_set>> mDescriptorSet;
	cgb::graphics_pipeline mPipeline;
	cgb::quake_camera mQuakeCam;

}; // model_loader_app

int main() // <== Starting point ==
{
	try {
		// What's the name of our application
		cgb::settings::gApplicationName = "cg_base::orca_loader";

		// Create a window and open it
		auto mainWnd = cgb::context().create_window("cg_base: ORCA Loader Example");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->set_presentaton_mode(cgb::presentation_mode::vsync);
		mainWnd->set_additional_back_buffer_attachments({ cgb::attachment::create_depth(cgb::image_format::default_depth_format()) });
		mainWnd->open(); 

		// Create an instance of vertex_buffers_app which, in this case,
		// contains the entire functionality of our application. 
		auto element = orca_loader_app();

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
	}
}


