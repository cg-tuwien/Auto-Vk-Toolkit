#include "imgui.h"

#include "configure_and_compose.hpp"
#include "imgui_manager.hpp"
#include "invokee.hpp"
#include "sequential_invoker.hpp"

#include "vk_convenience_functions.hpp"

class multi_invokee_rendering_app : public avk::invokee
{
public: 
	multi_invokee_rendering_app(avk::queue& aQueue, unsigned int aTrianglePart, int aExecutionOrder)
		: mQueue{ &aQueue }, mTrianglePart{ aTrianglePart }, mCustomExecutionOrder(aExecutionOrder)
	{}

	void initialize() override
	{
		auto wnd = avk::context().main_window();

		auto subpassDependencies = std::vector{
			avk::subpass_dependency(
				avk::subpass::external >> avk::subpass::index(0),
				avk::stage::color_attachment_output >> avk::stage::color_attachment_output,
				avk::access::color_attachment_write >> avk::access::color_attachment_read | avk::access::color_attachment_write // Layout transition is BOTH, read and write!
			),
			avk::subpass_dependency(
				avk::subpass::index(0) >> avk::subpass::external,
				avk::stage::color_attachment_output >> avk::stage::color_attachment_output,
				avk::access::color_attachment_write >> avk::access::color_attachment_read | avk::access::color_attachment_write
			)
		};

		mClearRenderPass = avk::context().create_renderpass({ 
				avk::attachment::declare(
					avk::format_from_window_color_buffer(avk::context().main_window()), 
					avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::color(0), avk::on_store::store
				) 
			}, subpassDependencies);

		mLoadRenderPass = avk::context().create_renderpass({
				avk::attachment::declare(
					avk::format_from_window_color_buffer(avk::context().main_window()),
					avk::on_load::load                                             , avk::usage::color(0), avk::on_store::store
				)
			}, subpassDependencies);
		
		// Create a graphics pipeline:
		mPipeline = avk::context().create_graphics_pipeline_for(
			avk::vertex_shader("shaders/a_triangle.vert"),
			avk::fragment_shader("shaders/a_triangle.frag"),
			avk::cfg::front_face::define_front_faces_to_be_clockwise(),
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)),
			mClearRenderPass,
			avk::push_constant_binding_data{ avk::shader_type::vertex, 0, sizeof(unsigned int) }
		);

		// set up updater
		// we want to use an updater, so create one:
		mUpdater.emplace();
		mPipeline.enable_shared_ownership(); // Make it usable with the updater
		mUpdater->on(
			avk::swapchain_resized_event(avk::context().main_window()),
			avk::shader_files_changed_event(mPipeline.as_reference())
		)
		.update(mPipeline);
	}

	void update() override
	{
		// On Esc pressed,
		if (avk::input().key_pressed(avk::key_code::escape) || avk::context().main_window()->should_be_closed()) {
			// stop the current composition:
			avk::current_composition()->stop();
		}
	}

	void render() override
	{
		auto mainWnd = avk::context().main_window();

		// need handling the case where previous invokees had been disabled (or this is the first invokee on the list)
		bool firstInvokeeInChain = !mainWnd->has_consumed_current_image_available_semaphore();

		// Get a command pool to allocate command buffers from:
		auto& commandPool = avk::context().get_command_pool_for_single_use_command_buffers(*mQueue);

		// Create a command buffer and render into the *current* swap chain image:
		auto cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		auto commands = avk::context().record({
			avk::command::push_constants(mPipeline->layout(), mTrianglePart, avk::shader_type::vertex),
			avk::command::render_pass(
				firstInvokeeInChain ? mClearRenderPass.as_reference() : mLoadRenderPass.as_reference(), 
				avk::context().main_window()->current_backbuffer_reference(), {
					avk::command::bind_pipeline(mPipeline.as_reference()),
					avk::command::draw(3u, 1u, 0u, 0u)
				}
			)
		});

		if (firstInvokeeInChain) {
			commands
				.into_command_buffer(cmdBfr)
				.then_submit_to(*mQueue)
				.waiting_for(mainWnd->consume_current_image_available_semaphore() >> avk::stage::color_attachment_output);
		}
		else {
			commands
				.into_command_buffer(cmdBfr)
				.then_submit_to(*mQueue);
		}
		
		mainWnd->handle_lifetime(std::move(cmdBfr));
	}
	
private: // v== Member variables ==v

	avk::queue* mQueue;
	unsigned int mTrianglePart = 0;
	int mCustomExecutionOrder;
	avk::renderpass mClearRenderPass;
	avk::renderpass mLoadRenderPass;
	avk::graphics_pipeline mPipeline;
	

}; // draw_a_triangle_app

int main() // <== Starting point ==
{
	int result = EXIT_FAILURE;
	try {
		// Create a window and open it
		auto mainWnd = avk::context().create_window("Multiple Invokees");
		mainWnd->set_resolution({ 800, 600 });
		mainWnd->enable_resizing(true);
		mainWnd->set_presentaton_mode(avk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = avk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->set_queue_family_ownership(singleQueue.family_index());
		mainWnd->set_present_queue(singleQueue);
		
		// Create instances of our invokee:		
		auto app1 = multi_invokee_rendering_app(singleQueue, 2, -1);
		auto app2 = multi_invokee_rendering_app(singleQueue, 1, -2);
		auto app3 = multi_invokee_rendering_app(singleQueue, 0, -3);
				
		// Create another element for drawing the UI with ImGui
		auto ui = avk::imgui_manager(singleQueue);
		
		auto apps = std::vector<multi_invokee_rendering_app*>{ &app1, &app2, &app3 };
		ui.add_callback([&apps]() {	

			ImGui::Begin("Hello, Multi-Invokee World!");
			ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
			ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
			ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);

			for (auto* app : apps) {
				bool isEnabled = app->is_enabled();
				std::string name = std::format("Disable/Enable Invokee [{}]", app->name());
				ImGui::Checkbox(name.c_str(), &isEnabled);
				if (isEnabled != app->is_enabled())
				{
					if (!isEnabled) app->disable();
					else app->enable();
				}
			}

			static std::vector<float> values;
			values.push_back(1000.0f / ImGui::GetIO().Framerate);
			if (values.size() > 90) {
				values.erase(values.begin());
			}
			ImGui::PlotLines("ms/frame", values.data(), static_cast<int>(values.size()), 0, nullptr, 0.0f, FLT_MAX, ImVec2(0.0f, 100.0f));
			ImGui::End();
		});
		

		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			avk::application_name("Auto-Vk-Toolkit Example: Multiple Invokees"),
			[](avk::validation_layers& config) {
				//config.enable_feature(vk::ValidationFeatureEnableEXT::eSynchronizationValidation);
				//config.enable_feature(vk::ValidationFeatureEnableEXT::eBestPractices);
			},
			// Pass windows:
			mainWnd,
			// Pass invokees:
			app1, app2, app3, ui
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


