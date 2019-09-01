#include <cg_base.hpp>

class draw_a_triangle_app : public cgb::cg_element
{
public: // v== cgb::cg_element overrides which will be invoked by the framework ==v

	void initialize() override
	{
		auto swapChainFormat = cgb::context().main_window()->swap_chain_image_format();
		mPipeline = cgb::graphics_pipeline_for(
			cgb::vertex_shader("shaders/a_triangle.vert"),
			cgb::fragment_shader("shaders/a_triangle.frag"),
			cgb::cfg::front_face::define_front_faces_to_be_clockwise(),
			cgb::cfg::viewport_depth_scissors_config::from_window(cgb::context().main_window()),
			cgb::attachment::create_color(swapChainFormat)
		);

		mCmdBfrs = record_command_buffers_for_all_in_flight_frames(cgb::context().main_window(), [&](cgb::command_buffer& _CommandBuffer, int64_t _InFlightIndex) {
			_CommandBuffer.begin_render_pass_for_window(cgb::context().main_window(), _InFlightIndex);

			_CommandBuffer.handle().bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline->handle());
			_CommandBuffer.handle().draw(3u, 1u, 0u, 0u);

			_CommandBuffer.end_render_pass();
		});
	}

	void render() override
	{
		// Alternate drawing between the command buffers:
		auto inFlightIndex = cgb::context().main_window()->in_flight_index_for_frame();
		submit_command_buffer_ref(mCmdBfrs[inFlightIndex]);
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

private: // v== Member variables ==v

	cgb::graphics_pipeline mPipeline;
	std::vector<cgb::command_buffer> mCmdBfrs;

}; // draw_a_triangle_app

int main() // <== Starting point ==
{
	try {
		// What's the name of our application
		cgb::settings::gApplicationName = "Hello, World!";

		// Create a window and open it
		auto mainWnd = cgb::context().create_window("Hello World Window");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->set_presentaton_mode(cgb::presentation_mode::vsync);
		mainWnd->open(); 

		// Create an instance of my_first_cgb_app which, in this case,
		// contains the entire functionality of our application. 
		auto element = draw_a_triangle_app();

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


