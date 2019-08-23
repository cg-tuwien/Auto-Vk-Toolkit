#include <cg_base.hpp>

class vertex_buffers_app : public cgb::cg_element
{
	struct data_for_draw_call
	{
		std::vector<glm::vec3> mPositions;
		std::vector<glm::vec2> mTexCoords;
		std::vector<glm::vec3> mNormals;
		std::vector<uint32_t> mIndices;

		cgb::vertex_buffer mPositionsBuffer;
		cgb::vertex_buffer mTexCoordsBuffer;
		cgb::vertex_buffer mNormalsBuffer;
		cgb::index_buffer mIndexBuffer;

		cgb::material mMaterial;
	};

	struct transformation_matrices {
		glm::mat4 mModelMatrix;
		glm::mat4 mProjViewMatrix;
	};

	void initialize() override
	{
		// Load a model from file:
		auto sponza = cgb::model_t::load_from_file("assets/sponza_structure.obj", aiProcess_Triangulate | aiProcess_PreTransformVertices);
		// Get all the different materials of the model:
		auto distinctMaterials = sponza->distinct_material_configs();

		mDrawCalls.reserve(distinctMaterials.size()); // Due to an internal but, all the buffers can't properly be moved right now => reserve as a workaround. Sorry, and thanks for your patience. :-S
		
		// The following might be a bit tedious still, but maybe it's not. For what it's worth, it is expressive.
		// The following loop gathers all the vertex and index data PER MATERIAL and constructs the buffers and materials.
		// Later, we'll use ONE draw call PER MATERIAL to draw the whole scene.
		std::vector<cgb::material_config> allMatCofigs;
		for (const auto& pair : distinctMaterials) {
			allMatCofigs.push_back(pair.first);
			auto& newElement = mDrawCalls.emplace_back();
			
			// 1. Gather all the vertex and index data from the sub meshes:
			for (auto index : pair.second) {
				cgb::append_indices_and_vertex_data(
					cgb::additional_index_data(	newElement.mIndices,	[&]() { return sponza->indices_for_mesh<uint32_t>(index);					} ),
					cgb::additional_vertex_data(newElement.mPositions,	[&]() { return sponza->positions_for_mesh(index);							} ),
					cgb::additional_vertex_data(newElement.mTexCoords,	[&]() { return sponza->texture_coordinates_for_mesh<glm::vec2>(index, 0);	} ),
					cgb::additional_vertex_data(newElement.mNormals,	[&]() { return sponza->normals_for_mesh(index);								} )
				);
			}
			
			// 2. Build all the buffers for the GPU
			// 2.1 Positions:
			newElement.mPositionsBuffer = cgb::create_and_fill(
				cgb::vertex_buffer_meta::create_from_data(newElement.mPositions),
				cgb::memory_usage::device,
				newElement.mPositions.data()
				// Handle the semaphore, if one gets created (which will happen 
				// since we've requested to upload the buffer to the device)
				/*[] (auto _Semaphore) { 
					cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
				}*/
			);
			// 2.2 Texture Coordinates:
			newElement.mTexCoordsBuffer = cgb::create_and_fill(
				cgb::vertex_buffer_meta::create_from_data(newElement.mTexCoords),
				cgb::memory_usage::device,
				newElement.mTexCoords.data()
				// Handle the semaphore, if one gets created (which will happen 
				// since we've requested to upload the buffer to the device)
				/*[] (auto _Semaphore) { 
					cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
				}*/
			);
			// 2.3 Normals:
			newElement.mNormalsBuffer = cgb::create_and_fill(
				cgb::vertex_buffer_meta::create_from_data(newElement.mNormals),
				cgb::memory_usage::device,
				newElement.mNormals.data()
				// Handle the semaphore, if one gets created (which will happen 
				// since we've requested to upload the buffer to the device)
				/*[] (auto _Semaphore) { 
					cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
				}*/
			);
			// 2.4 Indices:
			newElement.mIndexBuffer = cgb::create_and_fill(
				cgb::index_buffer_meta::create_from_data(newElement.mIndices),
				// Where to put our memory? => On the device
				cgb::memory_usage::device,
				// Pass pointer to the data:
				newElement.mIndices.data()
				// Handle the semaphore, if one gets created (which will happen 
				// since we've requested to upload the buffer to the device)
				/*[] (auto _Semaphore) { 
					cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
				}*/
			);
		}

		auto [gpuMaterials, imageSamplers] = cgb::convert_for_gpu_usage(allMatCofigs);
		mGpuMaterialData = std::move(gpuMaterials);
		mImageSamplers = std::move(imageSamplers);

		mMaterialBuffer.reserve(cgb::context().main_window()->number_of_concurrent_frames());
		for (int i = 0; i < cgb::context().main_window()->number_of_concurrent_frames(); ++i) {
			mMaterialBuffer.emplace_back(cgb::create_and_fill(
				cgb::uniform_buffer_meta::create_from_data(mGpuMaterialData[0]),
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
			cgb::binding(1, 0, static_cast<cgb::uniform_buffer_t&>(mMaterialBuffer[0])) // Just take any buffer, this is just for the layout
		);

		// The following is a bit ugly and needs to be abstracted sometime in the future. Sorry for that.
		// Right now it is neccessary to upload the resource descriptors to the GPU (the information about the uniform buffer, in particular).
		// This descriptor set will be used in render(). It is only created once to save memory/to make lifetime management easier.
		mDescriptorSet.reserve(cgb::context().main_window()->number_of_concurrent_frames());
		for (int i = 0; i < cgb::context().main_window()->number_of_concurrent_frames(); ++i) {
			mDescriptorSet.emplace_back(std::make_shared<cgb::descriptor_set>());
			*mDescriptorSet.back() = std::move(cgb::descriptor_set::create({ 
				cgb::binding(0, 0, mImageSamplers),
				cgb::binding(1, 0, static_cast<cgb::uniform_buffer_t&>(mMaterialBuffer[i])) 
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

		auto renderPassHandle = cgb::context().main_window()->renderpass_handle();
		auto extent = cgb::context().main_window()->swap_chain_extent();

		auto curIndex = cgb::context().main_window()->sync_index_for_frame();

		cmdbfr.begin_render_pass(renderPassHandle, cgb::context().main_window()->backbuffer_at_index(curIndex)->handle(), { 0, 0 }, extent);
		// Bind the pipeline
		cmdbfr.handle().bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline->handle());
			
		// Set the push constants:
		auto pushConstantsForThisFrame = transformation_matrices { 
			glm::scale(glm::vec3(0.01f)),							// <-- mModelMatrix
			mQuakeCam.projection_matrix() * mQuakeCam.view_matrix()	// <-- mProjViewMatrix
		};
		cmdbfr.handle().pushConstants(mPipeline->layout_handle(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(pushConstantsForThisFrame), &pushConstantsForThisFrame );

		// Set the descriptors of the uniform buffer
		cmdbfr.handle().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mPipeline->layout_handle(), 0, 
			mDescriptorSet[cgb::context().main_window()->sync_index_for_frame()]->number_of_descriptor_sets(),
			mDescriptorSet[cgb::context().main_window()->sync_index_for_frame()]->descriptor_sets_addr(), 
			0, nullptr);

		for (auto& drawCall : mDrawCalls) {
			// Bind the vertex input buffers in the right order (corresponding to the layout specifiers in the vertex shader)
			cmdbfr.handle().bindVertexBuffers(0u, {
				drawCall.mPositionsBuffer->buffer_handle(), 
				drawCall.mTexCoordsBuffer->buffer_handle(), 
				drawCall.mNormalsBuffer->buffer_handle()
			}, { 0, 0, 0 });

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

private:
	std::vector<cgb::material_gpu_data> mGpuMaterialData;
	std::vector<cgb::image_sampler> mImageSamplers;

	std::vector<data_for_draw_call> mDrawCalls;
	std::vector<cgb::uniform_buffer> mMaterialBuffer;
	std::vector<std::shared_ptr<cgb::descriptor_set>> mDescriptorSet;
	cgb::graphics_pipeline mPipeline;
	cgb::quake_camera mQuakeCam;
};

int main()
{
	try {
		// What's the name of our application
		cgb::settings::gApplicationName = "Hello, World!";

		// Create a window and open it
		auto mainWnd = cgb::context().create_window("Hello World Window");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->set_presentaton_mode(cgb::presentation_mode::vsync);
		mainWnd->open(); 

		// Create an instance of vertex_buffers_app which, in this case,
		// contains the entire functionality of our application. 
		auto element = vertex_buffers_app();

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


