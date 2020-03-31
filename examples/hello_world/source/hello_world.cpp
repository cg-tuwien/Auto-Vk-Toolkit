#include <cg_base.hpp>

class draw_a_triangle_app : public cgb::cg_element
{
public: // v== cgb::cg_element overrides which will be invoked by the framework ==v

	void initialize() override
	{
		// Create a graphics pipeline:
		mPipeline = cgb::graphics_pipeline_for(
			cgb::vertex_shader("shaders/a_triangle.vert"),
			cgb::fragment_shader("shaders/a_triangle.frag"),
			cgb::cfg::front_face::define_front_faces_to_be_clockwise(),
			cgb::cfg::viewport_depth_scissors_config::from_window(),
			cgb::attachment::create_color(cgb::image_format::from_window_color_buffer())
		);

		// Create command buffers, one per frame in flight; use a convenience function for creating and recording them:
		mCmdBfrs = record_command_buffers_for_all_in_flight_frames(cgb::context().main_window(), [&](cgb::command_buffer_t& commandBuffer, int64_t inFlightIndex) {
			commandBuffer.begin_render_pass_for_framebuffer(cgb::context().main_window(), inFlightIndex);
			commandBuffer.handle().bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline->handle());
			commandBuffer.handle().draw(3u, 1u, 0u, 0u);
			commandBuffer.end_render_pass();
		});
	}

	void render() override
	{
		// Draw using the command buffer which is associated to the current frame in flight-index:
		auto inFlightIndex = cgb::context().main_window()->in_flight_index_for_frame();
		submit_command_buffer_ref(mCmdBfrs[inFlightIndex]);
	}

	void update() override
	{
		// On H pressed,
		if (cgb::input().key_pressed(cgb::key_code::h)) {
			// log a message:
			LOG_INFO_EM("Hello cg_base!");
		}

		// On C pressed,
		if (cgb::input().key_pressed(cgb::key_code::c)) {
			// center the cursor:
			auto resolution = cgb::context().main_window()->resolution();
			cgb::context().main_window()->set_cursor_pos({ resolution[0] / 2.0, resolution[1] / 2.0 });
		}

		// On Esc pressed,
		if (cgb::input().key_pressed(cgb::key_code::escape)) {
			// stop the current composition:
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
		cgb::settings::gApplicationName = "Hello, cg_base World!";

		// Create a window and open it
		auto mainWnd = cgb::context().create_window("cg_base main window");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->set_presentaton_mode(cgb::presentation_mode::vsync);
		mainWnd->open(); 

		// Create an instance of our main cgb::element which contains all the functionality:
		auto element = draw_a_triangle_app();

		// Setup an application by providing (an) element(s) which will be invoked:
		auto helloWorld = cgb::setup(element);
		helloWorld.start();
	}
	catch (std::runtime_error& re)
	{
		LOG_ERROR_EM(re.what());
	}
}


