
#include "imgui.h"
#include "imgui_impl_vulkan.h"

#include "configure_and_compose.hpp"
#include "imgui_manager.hpp"
#include "invokee.hpp"
#include "sequential_invoker.hpp"

class draw_a_triangle_app : public avk::invokee
{
public: // v== avk::invokee overrides which will be invoked by the framework ==v
	draw_a_triangle_app(avk::queue& aQueue) : mQueue{ &aQueue }
	{}

	void initialize() override
	{
		// Print some information about the available memory on the selected physical device:
		avk::context().print_available_memory_types();
		
		// Create a graphics pipeline:
		mPipeline = avk::context().create_graphics_pipeline_for(
			avk::vertex_shader("shaders/a_triangle.vert"),
			avk::fragment_shader("shaders/a_triangle.frag"),
			avk::cfg::front_face::define_front_faces_to_be_clockwise(),
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)),
			// Just use the main window's renderpass for this pipeline:
			avk::context().main_window()->get_renderpass()
		);
		
		// We want to use an updater => gotta create one:
		mUpdater.emplace();
		mUpdater->on(
			avk::swapchain_resized_event(avk::context().main_window()), // In the case of window resizes,
			avk::shader_files_changed_event(mPipeline.as_reference())   // or in the case of changes to the shader files (hot reloading), ...
		)
		.update(mPipeline); // ... it will recreate the pipeline.		

		auto imguiManager = avk::current_composition()->element_by_type<avk::imgui_manager>();
		if(nullptr != imguiManager) {
			imguiManager->add_callback([this](){	
				bool isEnabled = this->is_enabled();
		        ImGui::Begin("Hello, world!");
				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
				ImGui::Checkbox("Enable/Disable invokee", &isEnabled);				
				if (isEnabled != this->is_enabled())
				{					
					if (!isEnabled) this->disable();
					else this->enable();					
				}
				static std::vector<float> values;
				values.push_back(1000.0f / ImGui::GetIO().Framerate);
		        if (values.size() > 90) {
			        values.erase(values.begin());
		        }
	            ImGui::PlotLines("ms/frame", values.data(), static_cast<int>(values.size()), 0, nullptr, 0.0f, FLT_MAX, ImVec2(0.0f, 100.0f));
		        ImGui::End();
			});
		}
	}

	void update() override
	{
		// On H pressed,
		if (avk::input().key_pressed(avk::key_code::h)) {
			// log a message:
			LOG_INFO_EM("Hello Auto-Vk-Toolkit!");
		}

		// On C pressed,
		if (avk::input().key_pressed(avk::key_code::c)) {
			// center the cursor:
			auto resolution = avk::context().main_window()->resolution();
			avk::context().main_window()->set_cursor_pos({ resolution[0] / 2.0, resolution[1] / 2.0 });
		}

		// On Esc pressed,
		if (avk::input().key_pressed(avk::key_code::escape) || avk::context().main_window()->should_be_closed()) {
			// stop the current composition:
			avk::current_composition()->stop();
		}
	}

	/**	Render callback which is invoked by the framework every frame after every update() callback has been invoked.
	 *
	 *	Important: We must establish a dependency to the "swapchain image available" condition, i.e., we must wait for the
	 *	           next swap chain image to become available before we may start to render into it.
	 *			   This dependency is expressed through a semaphore, and the framework demands us to use it via the function:
	 *			   context().main_window()->consume_current_image_available_semaphore() for the main_window (our only window).
	 *
	 *			   More background information: At one point, we also must tell the presentation engine when we are done
	 *			   with rendering by the means of a semaphore. Actually, we would have to use the framework function:
	 *			   mainWnd->add_present_dependency_for_current_frame() for that purpose, but we don't have to do it in our case
	 *			   since we are rendering a GUI. imgui_manager will add a semaphore as dependency for the presentation engine.
	 */
	void render() override
	{
		auto mainWnd = avk::context().main_window();

		// The main window's swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();

		// Get a command pool to allocate command buffers from:
		auto& commandPool = avk::context().get_command_pool_for_single_use_command_buffers(*mQueue);

		// Create a command buffer and render into the *current* swap chain image:
		auto cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		
		avk::context().record({
				// Begin and end one renderpass:
				avk::command::render_pass(mPipeline->renderpass_reference(), avk::context().main_window()->current_backbuffer_reference(), {
					// And within, bind a pipeline and draw three vertices:
					avk::command::bind_pipeline(mPipeline.as_reference()),
					avk::command::draw(3u, 1u, 0u, 0u)
				})
			})
			.into_command_buffer(cmdBfr)
			.then_submit_to(*mQueue)
			.waiting_for(imageAvailableSemaphore >> avk::stage::color_attachment_output)
			.submit();

		// Use a convenience function of avk::window to take care of the command buffer's lifetime:
		// It will get deleted in the future after #concurrent-frames have passed by.
		avk::context().main_window()->handle_lifetime(std::move(cmdBfr));
	}

private: // v== Member variables ==v

	avk::queue* mQueue;
	avk::graphics_pipeline mPipeline;

}; // draw_a_triangle_app

int main() // <== Starting point ==
{
	int result = EXIT_FAILURE;
	try {
		// Create a window and open it
		auto mainWnd = avk::context().create_window("Hello World");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->enable_resizing(true);
		mainWnd->set_presentaton_mode(avk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->set_number_of_presentable_images(3u);
		mainWnd->open();

		auto& singleQueue = avk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->set_queue_family_ownership(singleQueue.family_index());
		mainWnd->set_present_queue(singleQueue);
		
		// Create an instance of our main "invokee" which contains all the functionality:
		auto app = draw_a_triangle_app(singleQueue);
		// Create another invokee for drawing the UI with ImGui
		auto ui = avk::imgui_manager(singleQueue);

		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			avk::application_name("Hello, Auto-Vk-Toolkit World!"),
			[](avk::validation_layers& config) {
				config.enable_feature(vk::ValidationFeatureEnableEXT::eSynchronizationValidation);
			},
			// Pass windows:
			mainWnd,
			// Pass invokees:
			app, ui
		);

		// Create an invoker object, which defines the way how invokees/elements are invoked
		// (In this case, just sequentially in their execution order):
		avk::sequential_invoker invoker;

		// With everything configured, let us start our render loop:
		composition.start_render_loop(
			// Callback in the case of update:
			[&invoker](const std::vector<avk::invokee*>& aToBeInvoked) {
				// Call all the update() callbacks:
				invoker.invoke_updates(aToBeInvoked);
			},
			// Callback in the case of render:
			[&invoker](const std::vector<avk::invokee*>& aToBeInvoked) {
				// Sync (wait for fences and so) per window BEFORE executing render callbacks
				avk::context().execute_for_each_window([](avk::window* wnd) {
					wnd->sync_before_render();
				});

				// Call all the render() callbacks:
				invoker.invoke_renders(aToBeInvoked);

				// Render per window:
				avk::context().execute_for_each_window([](avk::window* wnd) {
					wnd->render_frame();
				});
			}
		); // This is a blocking call, which loops until avk::current_composition()->stop(); has been called (see update())
	
		result = EXIT_SUCCESS;
	}
	catch (avk::logic_error&) {}
	catch (avk::runtime_error&) {}
	return result;
}
