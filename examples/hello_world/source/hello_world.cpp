#include <exekutor.hpp>
#include <imgui.h>

class draw_a_triangle_app : public xk::cg_element
{
public: // v== cgb::cg_element overrides which will be invoked by the framework ==v
	draw_a_triangle_app(ak::queue& aQueue) : mQueue{ &aQueue }
	{}
	
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
			LOG_INFO_EM("Hello Exekutor! Hello Auto-Vk!");
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

		// Get a command pool to allocate command buffers from:
		auto& commandPool = xk::context().get_command_pool_for_single_use_command_buffers(*mQueue);

		// Create a command buffer and render into the *current* swap chain image:
		auto cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		cmdBfr->begin_recording();
		cmdBfr->begin_render_pass_for_framebuffer(mPipeline->get_renderpass(), xk::context().main_window()->current_backbuffer());
		cmdBfr->handle().bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline->handle());
		cmdBfr->handle().draw(3u, 1u, 0u, 0u);
		cmdBfr->end_render_pass();
		cmdBfr->end_recording();

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto& imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();

		// Submit the draw call (that will be executed after the semaphore has been signalled:
		auto renderFinishedSemaphore = mQueue->submit_with_semaphore(cmdBfr, imageAvailableSemaphore);

		// Present must not happen before rendering is finished => add renderFinishedSemaphore as a dependency:
		mainWnd->add_render_finished_semaphore_for_current_frame(std::move(renderFinishedSemaphore));
		mainWnd->handle_lifetime(std::move(cmdBfr));
	}

private: // v== Member variables ==v

	ak::queue* mQueue;
	ak::graphics_pipeline mPipeline;

}; // draw_a_triangle_app

int main() // <== Starting point ==
{
	try {
		// Create a window and open it
		auto mainWnd = xk::context().create_window("cg_base main window");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->set_presentaton_mode(xk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = xk::context().create_queue({}, ak::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->add_queue_family_ownership(singleQueue);
		mainWnd->set_present_queue(singleQueue);
		
		// Create an instance of our main cgb::element which contains all the functionality:
		auto app = draw_a_triangle_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = xk::imgui_manager(singleQueue);

		// GO:
		xk::execute(
			xk::application_name("Hello, Exekutor + Auto-Vk World!"),
			mainWnd,
			app,
			ui
		);		
	}
	catch (xk::logic_error&) {}
	catch (xk::runtime_error&) {}
	catch (ak::logic_error&) {}
	catch (ak::runtime_error&) {}
}


