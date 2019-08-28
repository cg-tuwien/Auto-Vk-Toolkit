#include <cg_base.hpp>

class vertex_buffers_app : public cgb::cg_element
{
	void initialize() override
	{
		// Create vertex buffers, but don't upload vertices yet; we'll do that in the render() method.
		// Create multiple vertex buffers because we'll update the data every frame and frames run concurrently.
		mVertexBuffers.reserve(cgb::context().main_window()->number_of_in_flight_frames()); // TODO: Comment and it will crash!
		for (size_t i = 0; i < cgb::context().main_window()->number_of_in_flight_frames(); ++i) {
			mVertexBuffers.push_back(
				// We want our buffer to "live" in GPU memory
				cgb::create(cgb::vertex_buffer_meta::create_from_data(mVertexData),	cgb::memory_usage::device)
			);
		}

		// Create and upload the indices now, since we won't change them ever:
		mIndexBuffer = cgb::create_and_fill(
			// Also this buffer should "live" in GPU memory
			cgb::index_buffer_meta::create_from_data(mIndices),	cgb::memory_usage::device,
			// We're calling `create_and_fill`, so already pass a pointer to the data:
			mIndices.data(),
			// Handle the semaphore, if one gets created (which will actually happen in 
			// this case since we've requested to upload the buffer to the device)
			[] (auto _Semaphore) { 
				cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
			}
		);

		auto swapChainFormat = cgb::context().main_window()->swap_chain_image_format();
		// Create our rasterization graphics pipeline with the required configuration:
		mPipeline = cgb::graphics_pipeline_for(
			cgb::vertex_input_location(0, &Vertex::pos),
			cgb::vertex_input_location(1, &Vertex::color),
			"shaders/passthrough.vert",
			"shaders/color.frag",
			cgb::cfg::front_face::define_front_faces_to_be_clockwise(),
			cgb::cfg::viewport_depth_scissors_config::from_window(cgb::context().main_window()),
			//cgb::renderpass(cgb::renderpass_t::create_good_renderpass((VkFormat)cgb::context().main_window()->swap_chain_image_format().mFormat))
			//std::get<std::shared_ptr<cgb::renderpass_t>>(cgb::context().main_window()->getrenderpass())
			cgb::attachment::create_color(swapChainFormat)
		);

		mCmdBfrs = record_command_buffers_for_all_in_flight_frames(cgb::context().main_window(), [&](cgb::command_buffer& _CommandBuffer, int64_t _InFlightIndex) {
			_CommandBuffer.begin_render_pass_for_window(cgb::context().main_window(), _InFlightIndex);

			_CommandBuffer.handle().bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline->handle());
			cgb::context().draw_indexed(mPipeline, _CommandBuffer, mVertexBuffers[_InFlightIndex], mIndexBuffer);

			_CommandBuffer.end_render_pass();
		});
	}

	void render() override
	{
		// Modify our vertex data according to our rotation animation and upload this frame's vertex data:
		auto rotAngle = glm::radians(90.0f) * cgb::time().time_since_start();
		auto rotMatrix = glm::rotate(rotAngle, glm::vec3(0.f, 1.f, 0.f));
		auto translateZ = glm::translate(glm::vec3{ 0.0f, 0.0f, -0.5f });
		auto invTranslZ = glm::inverse(translateZ);

		std::vector<Vertex> vertexDataCurrentFrame;
		for (const Vertex& vtx : mVertexData) {
			glm::vec4 origPos{ vtx.pos, 1.0f };
			vertexDataCurrentFrame.push_back({
				glm::vec3(invTranslZ * rotMatrix * translateZ * origPos),
				vtx.color
			});
		}

		auto curIndex = cgb::context().main_window()->in_flight_index_for_frame();

		cgb::fill(
			mVertexBuffers[curIndex],
			vertexDataCurrentFrame.data(), 
			[] (auto _Semaphore) { 
				cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_Semaphore)); 
			}
		);

		submit_command_buffer_ref(mCmdBfrs[curIndex]);
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
	}

private:
	// Define a struct for our vertex input data:
	struct Vertex {
	    glm::vec3 pos;
	    glm::vec3 color;
	};

	// Vertex data for drawing a pyramid:
	const std::vector<Vertex> mVertexData = {
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

	// Indices for the faces (triangles) of the pyramid:
	const std::vector<uint16_t> mIndices = {
		 0, 1, 2,  3, 4, 5,  6, 7, 8,  9, 10, 11
	};

	std::vector<cgb::vertex_buffer> mVertexBuffers;
	cgb::index_buffer mIndexBuffer;
	cgb::graphics_pipeline mPipeline;
	std::vector<cgb::command_buffer> mCmdBfrs;
};

int main()
{
	try {
		// What's the name of our application
		cgb::settings::gApplicationName = "Hello, World!";
		cgb::settings::gQueueSelectionPreference = cgb::device_queue_selection_strategy::prefer_everything_on_single_queue;

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


