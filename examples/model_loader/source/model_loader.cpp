#include <cg_base.hpp>

class model_loader_app : public cgb::cg_element
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

		int mMaterialIndex;
	};

	struct transformation_matrices {
		glm::mat4 mModelMatrix;
		glm::mat4 mProjViewMatrix;
		int mMaterialIndex;
	};

public: // v== cgb::cg_element overrides which will be invoked by the framework ==v

	void initialize() override
	{
		mInitTime = std::chrono::high_resolution_clock::now();

		// Load a model from file:
		auto sponza = cgb::model_t::load_from_file("assets/sponza_structure.obj", aiProcess_Triangulate | aiProcess_PreTransformVertices);
		// Get all the different materials of the model:
		auto distinctMaterials = sponza->distinct_material_configs();

		// The following might be a bit tedious still, but maybe it's not. For what it's worth, it is expressive.
		// The following loop gathers all the vertex and index data PER MATERIAL and constructs the buffers and materials.
		// Later, we'll use ONE draw call PER MATERIAL to draw the whole scene.
		std::vector<cgb::material_config> allMatConfigs;
		for (const auto& pair : distinctMaterials) {
			auto& newElement = mDrawCalls.emplace_back();
			allMatConfigs.push_back(pair.first);
			newElement.mMaterialIndex = static_cast<int>(allMatConfigs.size() - 1);

			// 1. Gather all the vertex and index data from the sub meshes:
			for (auto index : pair.second) {
				cgb::append_indices_and_vertex_data(
					cgb::additional_index_data(	newElement.mIndices,	[&]() { return sponza->indices_for_mesh<uint32_t>(index);								} ),
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
				newElement.mPositions.data(),
				cgb::sync::with_barriers_on_current_frame() // TODO: I wonder why validation layers do not complain here, but.... 
			);
			// 2.2 Texture Coordinates:
			newElement.mTexCoordsBuffer = cgb::create_and_fill(
				cgb::vertex_buffer_meta::create_from_data(newElement.mTexCoords),
				cgb::memory_usage::device,
				newElement.mTexCoords.data(),
				cgb::sync::with_barriers_on_current_frame()
			);
			// 2.3 Normals:
			newElement.mNormalsBuffer = cgb::create_and_fill(
				cgb::vertex_buffer_meta::create_from_data(newElement.mNormals),
				cgb::memory_usage::device,
				newElement.mNormals.data(),
				cgb::sync::with_barriers_on_current_frame()
			);
			// 2.4 Indices:
			newElement.mIndexBuffer = cgb::create_and_fill(
				cgb::index_buffer_meta::create_from_data(newElement.mIndices),
				// Where to put our memory? => On the device
				cgb::memory_usage::device,
				// Pass pointer to the data:
				newElement.mIndices.data(),
				cgb::sync::with_barriers_on_current_frame()
			);
		}

		// For all the different materials, transfer them in structs which are well 
		// suited for GPU-usage (proper alignment, and containing only the relevant data),
		// also load all the referenced images from file and provide access to them
		// via samplers; It all happens in `cgb::convert_for_gpu_usage`:
		auto [gpuMaterials, imageSamplers] = cgb::convert_for_gpu_usage(
			allMatConfigs, 
			cgb::image_usage::read_only_sampled_image,
			cgb::filter_mode::bilinear,
			cgb::border_handling_mode::repeat,
			cgb::sync::with_barriers_on_current_frame() // TODO: ....they complain here, if I use with_barriers_on_current_frame()
		);
		
		mMaterialBuffer = cgb::create_and_fill(
			cgb::storage_buffer_meta::create_from_data(gpuMaterials),
			cgb::memory_usage::host_coherent,
			gpuMaterials.data(),
			cgb::sync::not_required()
		);
		
		mImageSamplers = std::move(imageSamplers);

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
			cgb::binding(1, 0, mMaterialBuffer)
		);

		// Add the camera to the composition (and let it handle the updates)
		mQuakeCam.set_translation({ 0.0f, 0.0f, 0.0f });
		mQuakeCam.set_perspective_projection(glm::radians(60.0f), cgb::context().main_window()->aspect_ratio(), 0.5f, 100.0f);
		//mQuakeCam.set_orthographic_projection(-5, 5, -5, 5, 0.5, 100);
		cgb::current_composition().add_element(mQuakeCam);
	}

	void render() override
	{
		auto cmdbfr = cgb::context().graphics_queue().create_single_use_command_buffer();
		cmdbfr->begin_recording();
		cmdbfr->begin_render_pass_for_framebuffer(cgb::context().main_window());
		cmdbfr->bind_pipeline(mPipeline);
		cmdbfr->bind_descriptors(mPipeline->layout(), { 
				cgb::binding(0, 0, mImageSamplers),
				cgb::binding(1, 0, mMaterialBuffer)
			}
		);

		for (auto& drawCall : mDrawCalls) {
			// Bind the vertex input buffers in the right order (corresponding to the layout specifiers in the vertex shader)
			cmdbfr->handle().bindVertexBuffers(0u, {
				drawCall.mPositionsBuffer->buffer_handle(), 
				drawCall.mTexCoordsBuffer->buffer_handle(), 
				drawCall.mNormalsBuffer->buffer_handle()
			}, { 0, 0, 0 });

			// Set the push constants:
			auto pushConstantsForThisDrawCall = transformation_matrices { 
				glm::scale(glm::vec3(0.01f)),							// <-- mModelMatrix
				mQuakeCam.projection_matrix() * mQuakeCam.view_matrix(),// <-- mProjViewMatrix
				drawCall.mMaterialIndex
			};
			cmdbfr->handle().pushConstants(mPipeline->layout_handle(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(pushConstantsForThisDrawCall), &pushConstantsForThisDrawCall);

			// Bind and use the index buffer to create the draw call:
			vk::IndexType indexType = cgb::to_vk_index_type(drawCall.mIndexBuffer->meta_data().sizeof_one_element());
			cmdbfr->handle().bindIndexBuffer(drawCall.mIndexBuffer->buffer_handle(), 0u, indexType);
			cmdbfr->handle().drawIndexed(drawCall.mIndexBuffer->meta_data().num_elements(), 1u, 0u, 0u, 0u);
		}

		cmdbfr->end_render_pass();
		cmdbfr->end_recording();
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

	cgb::storage_buffer mMaterialBuffer;
	std::vector<cgb::image_sampler> mImageSamplers;

	std::vector<data_for_draw_call> mDrawCalls;
	cgb::graphics_pipeline mPipeline;
	cgb::quake_camera mQuakeCam;

}; // model_loader_app

int main() // <== Starting point ==
{
	try {
		// What's the name of our application
		cgb::settings::gApplicationName = "Model Loader Example";
		cgb::settings::gLoadImagesInSrgbFormatByDefault = true;
		cgb::settings::gQueueSelectionPreference = cgb::device_queue_selection_strategy::prefer_everything_on_single_queue;

		// Create a window and open it
		auto mainWnd = cgb::context().create_window("Hello World Window");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->set_presentaton_mode(cgb::presentation_mode::vsync);
		mainWnd->set_additional_back_buffer_attachments({ cgb::attachment::create_depth(cgb::image_format::default_depth_format()) });
		mainWnd->request_srgb_framebuffer(true);
		mainWnd->open(); 

		// Create an instance of vertex_buffers_app which, in this case,
		// contains the entire functionality of our application. 
		auto element = model_loader_app();

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


