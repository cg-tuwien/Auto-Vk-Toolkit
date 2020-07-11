#include <exekutor.hpp>
#include <imgui.h>

class draw_a_triangle_app : public xk::cg_element
{
public: // v== cgb::cg_element overrides which will be invoked by the framework ==v

	void initialize() override
	{
		// Create a graphics pipeline:
		mPipeline = xk::context().create_graphics_pipeline_for(
			ak::vertex_shader("shaders/a_triangle.vert"),
			ak::fragment_shader("shaders/a_triangle.frag"),
			ak::cfg::front_face::define_front_faces_to_be_clockwise(),
			ak::cfg::viewport_depth_scissors_config::from_framebuffer(xk::context().main_window()->backbuffer_at_index(0)),
			ak::attachment::declare(xk::format_from_window_color_buffer(xk::context().main_window()), ak::on_load::clear, ak::color(0), ak::on_store::store) // But not in presentable format, because ImGui comes after
		);

		// Create command buffers, one per frame in flight; use a convenience function for creating and recording them:
		const auto framesInFlight = xk::context().main_window()->number_of_in_flight_frames();
		auto& commandPool = xk::context().get_command_pool_for_reusable_command_buffers(xk::context().graphics_queue());
		mCmdBfrs = commandPool->alloc_command_buffers(framesInFlight);
		for (size_t i = 0; i < framesInFlight; ++i) {
			mCmdBfrs[i]->begin_recording();
			mCmdBfrs[i]->begin_render_pass_for_framebuffer(mPipeline->get_renderpass(), xk::context().main_window()->backbufer_for_frame(i)); // TODO: only because #concurrent == #present... This is not optimal
			mCmdBfrs[i]->handle().bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline->handle());
			mCmdBfrs[i]->handle().draw(3u, 1u, 0u, 0u);
			mCmdBfrs[i]->end_render_pass();
			mCmdBfrs[i]->end_recording();
		}

		auto imguiManager = xk::current_composition()->element_by_type<xk::imgui_manager>();
		if(nullptr != imguiManager) {
			imguiManager->add_callback([](){
				
		        ImGui::Begin("Hello, world!");
				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);

				static std::vector<float> values;
				values.push_back(1000.0f / ImGui::GetIO().Framerate);
		        if (values.size() > 90) {
			        values.erase(values.begin());
		        }
	            ImGui::PlotLines("ms/frame", values.data(), values.size(), 0, nullptr, 0.0f, FLT_MAX, ImVec2(0.0f, 100.0f));
		        ImGui::End();
			});
		}
	}

	void update() override
	{
		// On H pressed,
		if (xk::input().key_pressed(xk::key_code::h)) {
			// log a message:
			LOG_INFO_EM("Hello cg_base!");
		}

		// On C pressed,
		if (xk::input().key_pressed(xk::key_code::c)) {
			// center the cursor:
			auto resolution = xk::context().main_window()->resolution();
			xk::context().main_window()->set_cursor_pos({ resolution[0] / 2.0, resolution[1] / 2.0 });
		}

		// On Esc pressed,
		if (xk::input().key_pressed(xk::key_code::escape)) {
			// stop the current composition:
			xk::current_composition()->stop();
		}
	}

	void render() override
	{
		auto mainWnd = xk::context().main_window();
		
		// Draw using the command buffer which is associated to the current frame in flight-index:
		auto inFlightIndex = mainWnd->in_flight_index_for_frame();
		mainWnd->submit_for_backbuffer_ref(mCmdBfrs[inFlightIndex]);
	}

private: // v== Member variables ==v

	ak::graphics_pipeline mPipeline;
	std::vector<ak::command_buffer> mCmdBfrs;

}; // draw_a_triangle_app

int main() // <== Starting point ==
{
	try {
		// What's the name of our application
		xk::settings::gApplicationName = "Hello, cg_base World!";

		// Create a window and open it
		auto mainWnd = xk::context().create_window("cg_base main window");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->set_presentaton_mode(xk::presentation_mode::immediate);
		mainWnd->open();
		
		// Create an instance of our main cgb::element which contains all the functionality:
		auto app = draw_a_triangle_app();
		// Create another element for drawing the UI with ImGui
		auto ui = xk::imgui_manager();
		
		// Setup an application by providing elements which will be invoked:
		auto helloWorld = xk::setup(app, ui);
		helloWorld.start();

		xk::execute(
			xk::application_name("Hello, Exekutor + Auto-Vk World!"),
			mainWnd,
			xk::request_queues([](auto physicalDevice) {
				
			}),
			app,
			ui,
		);
		
	}
	catch (xk::logic_error&) {}
	catch (xk::runtime_error&) {}
	catch (ak::logic_error&) {}
	catch (ak::runtime_error&) {}
}


