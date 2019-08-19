#include <cg_base.hpp>

class vertex_buffers_app : public cgb::cg_element
{
	void initialize() override
	{
		auto sponza = cgb::model_t::load_from_file("assets/sponza_structure.obj", aiProcess_Triangulate | aiProcess_PreTransformVertices);
		auto allMeshes = sponza->select_all_meshes();
		auto positions = sponza->positions_for_meshes(allMeshes);
		auto colors = sponza->normals_for_meshes(allMeshes);
		auto indices = sponza->indices_for_meshes<uint32_t>(allMeshes); // TODO: Shift indices when combining the meshes!!

		mPositionsBuffer = cgb::create_and_fill(
			cgb::vertex_buffer_meta::create_from_data(positions),
			cgb::memory_usage::device,
			positions.data(),
			// Handle the semaphore, if one gets created (which will happen 
			// since we've requested to upload the buffer to the device)
			[] (auto _Semaphore) { 
				cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
			}
		);
		
		mColorsBuffer = cgb::create_and_fill(
			cgb::vertex_buffer_meta::create_from_data(colors),
			cgb::memory_usage::device,
			colors.data(),
			// Handle the semaphore, if one gets created (which will happen 
			// since we've requested to upload the buffer to the device)
			[] (auto _Semaphore) { 
				cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
			}
		);
		
		// Create and upload the indices now, since we won't change them ever:
		mIndexBuffer = cgb::create_and_fill(
			cgb::index_buffer_meta::create_from_data(indices),
			// Where to put our memory? => On the device
			cgb::memory_usage::device,
			// Pass pointer to the data:
			indices.data(),
			// Handle the semaphore, if one gets created (which will happen 
			// since we've requested to upload the buffer to the device)
			[] (auto _Semaphore) { 
				cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
			}
		);

		auto swapChainFormat = cgb::context().main_window()->swap_chain_image_format();
		// Create our rasterization graphics pipeline with the required configuration:
		mPipeline = cgb::graphics_pipeline_for(
			cgb::vertex_input_location(0, positions[0]).from_buffer_at_binding(0),
			cgb::vertex_input_location(1, colors[0]).from_buffer_at_binding(1),
			"shaders/passthrough.vert",
			"shaders/color.frag",
			cgb::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			cgb::cfg::viewport_depth_scissors_config::from_window(cgb::context().main_window()),
			//cgb::renderpass(cgb::renderpass_t::create_good_renderpass((VkFormat)cgb::context().main_window()->swap_chain_image_format().mFormat))
			//std::get<std::shared_ptr<cgb::renderpass_t>>(cgb::context().main_window()->getrenderpass())
			cgb::attachment::create_color(swapChainFormat),
			cgb::attachment::create_depth(),
			//cgb::attachment::create_depth(cgb::image_format{ vk::Format::eD32Sfloat }),
			cgb::push_constant_binding_data { cgb::shader_type::vertex, 0, sizeof(glm::mat4) }
		);

			
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
		cmdbfr.handle().bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline->handle());
		
		auto thePushConstant = mQuakeCam.projection_matrix() * mQuakeCam.view_matrix() * glm::scale(glm::vec3(0.01f));
		cmdbfr.handle().pushConstants(mPipeline->layout_handle(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(thePushConstant), glm::value_ptr(thePushConstant) );

		cmdbfr.handle().bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline->handle());
		cmdbfr.handle().bindVertexBuffers(0u, { mPositionsBuffer->buffer_handle(), mColorsBuffer->buffer_handle() }, { 0, 0 });
		vk::IndexType indexType = cgb::to_vk_index_type(mIndexBuffer->meta_data().sizeof_one_element());
		cmdbfr.handle().bindIndexBuffer(mIndexBuffer->buffer_handle(), 0u, indexType);
		cmdbfr.handle().drawIndexed(mIndexBuffer->meta_data().num_elements(), 1u, 0u, 0u, 0u);

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
	cgb::vertex_buffer mPositionsBuffer;
	cgb::vertex_buffer mColorsBuffer;
	cgb::index_buffer mIndexBuffer;
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


