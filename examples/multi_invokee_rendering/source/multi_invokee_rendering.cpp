#include <gvk.hpp>
#define IMGUI_DISABLE_OBSOLETE_KEYIO
#include <imgui.h>

class multi_invokee_rendering_app : public gvk::invokee
{
public: // v== cgb::invokee overrides which will be invoked by the framework ==v
	multi_invokee_rendering_app(avk::queue& aQueue, unsigned int aTrianglePart, int aExecutionOrder = 0)
		: gvk::invokee(aExecutionOrder), mQueue{ &aQueue }, mTrianglePart{ aTrianglePart }
	{}

	void initialize() override
	{		
		auto wnd = gvk::context().main_window();

		std::vector<avk::attachment> attachments;
		attachments.push_back(avk::attachment::declare(gvk::format_from_window_color_buffer(gvk::context().main_window()), avk::on_load::load, avk::color(0), avk::on_store::store)); // But not in presentable format, because ImGui (and other invokees) comes after);
		for (auto a : wnd->get_additional_back_buffer_attachments()) {
			a.mLoadOperation = avk::on_load::dont_care;
			a.mStoreOperation = avk::on_store::dont_care;
			attachments.push_back(a);
		}
		mRenderPass = gvk::context().create_renderpass(
			attachments,
			[](avk::renderpass_sync& rpSync) {
				if (rpSync.is_external_pre_sync()) {
					rpSync.mSourceStage = avk::pipeline_stage::color_attachment_output;
					rpSync.mSourceMemoryDependency = avk::memory_access::color_attachment_write_access;
					rpSync.mDestinationStage = avk::pipeline_stage::color_attachment_output;
					rpSync.mDestinationMemoryDependency = avk::memory_access::color_attachment_read_access;
				}
				if (rpSync.is_external_post_sync()) {
					rpSync.mSourceStage = avk::pipeline_stage::color_attachment_output;
					rpSync.mSourceMemoryDependency = avk::memory_access::color_attachment_write_access;
					rpSync.mDestinationStage = avk::pipeline_stage::bottom_of_pipe;
					rpSync.mDestinationMemoryDependency = {};
				}
			}
		);


		// Create a graphics pipeline:
		mPipeline = gvk::context().create_graphics_pipeline_for(
			avk::vertex_shader("shaders/a_triangle.vert"),
			avk::fragment_shader("shaders/a_triangle.frag"),
			avk::cfg::front_face::define_front_faces_to_be_clockwise(),
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(gvk::context().main_window()->backbuffer_at_index(0)),
			avk::attachment::declare(gvk::format_from_window_color_buffer(gvk::context().main_window()), avk::on_load::load, avk::color(0), avk::on_store::store),
			avk::push_constant_binding_data{ avk::shader_type::vertex, 0, sizeof(unsigned int) }
		);

		// set up updater
		// we want to use an updater, so create one:
		mUpdater.emplace();
		mPipeline.enable_shared_ownership(); // Make it usable with the updater
		mUpdater->on(
				gvk::swapchain_resized_event(gvk::context().main_window()),
				gvk::shader_files_changed_event(mPipeline)
			)
			.update(mPipeline);
	}

	void update() override
	{
		// On Esc pressed,
		if (gvk::input().key_pressed(gvk::key_code::escape)) {
			// stop the current composition:
			gvk::current_composition()->stop();
		}
	}

	void render() override
	{
		auto mainWnd = gvk::context().main_window();

		// need handling the case where previous invokees had been disabled (or this is the first invokee on the list)
		bool firstInvokeeInChain = !mainWnd->has_consumed_current_image_available_semaphore();

		// Get a command pool to allocate command buffers from:
		auto& commandPool = gvk::context().get_command_pool_for_single_use_command_buffers(*mQueue);

		// Create a command buffer and render into the *current* swap chain image:
		auto cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		cmdBfr->begin_recording();

		const auto pushConstants = mTrianglePart;
		cmdBfr->handle().pushConstants(mPipeline->layout_handle(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(pushConstants), &pushConstants);
		
		// first invokee needs to clear up the backbuffer image: main window render pass clears on load
		cmdBfr->begin_render_pass_for_framebuffer(firstInvokeeInChain ? gvk::context().main_window()->get_renderpass() : const_referenced(mRenderPass.value()), gvk::context().main_window()->current_backbuffer());
		cmdBfr->handle().bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline->handle());
		cmdBfr->handle().draw(3u, 1u, 0u, 0u);
		cmdBfr->end_render_pass();
		cmdBfr->end_recording();
		
		if (!firstInvokeeInChain) {
			mQueue->submit(cmdBfr);
			mainWnd->handle_lifetime(avk::owned(cmdBfr));
		}
		else
		{
			auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();
			mQueue->submit(cmdBfr, imageAvailableSemaphore);
			mainWnd->handle_lifetime(avk::owned(cmdBfr));
		}
	}
	
private: // v== Member variables ==v

	unsigned int mTrianglePart = 0;
	std::optional<avk::renderpass> mRenderPass;
	avk::queue* mQueue;
	avk::graphics_pipeline mPipeline;
	

}; // draw_a_triangle_app

int main() // <== Starting point ==
{
	try {
		// Create a window and open it
		auto mainWnd = gvk::context().create_window("Hello Multi-Invokee World");
		mainWnd->set_resolution({ 800, 600 });
		mainWnd->enable_resizing(true);
		mainWnd->set_presentaton_mode(gvk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = gvk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->add_queue_family_ownership(singleQueue);
		mainWnd->set_present_queue(singleQueue);
		
		// Create instances of our invokee:		
		auto app1 = multi_invokee_rendering_app(singleQueue, 2, -1);
		auto app2 = multi_invokee_rendering_app(singleQueue, 1, -2);
		auto app3 = multi_invokee_rendering_app(singleQueue, 0, -3);
				
		// Create another element for drawing the UI with ImGui
		auto ui = gvk::imgui_manager(singleQueue);
		
		auto apps = std::vector<multi_invokee_rendering_app*>{ &app1, &app2, &app3 };
		ui.add_callback([&apps]() {	

			ImGui::Begin("Hello, Multi-Invokee World!");
			ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
			ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
			ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);

			for (auto* app : apps)
			{
				bool isEnabled = app->is_enabled();
				std::string name = fmt::format("Disable/Enable Invokee [{}]", app->name());
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
		

		// GO:
		gvk::start(
			gvk::application_name("Hello, Gears-Vk + Auto-Vk World!"),
			mainWnd,
			app1,
			app2,
			app3,
			ui
		);			
	}
	catch (gvk::logic_error&) {}
	catch (gvk::runtime_error&) {}
	catch (avk::logic_error&) {}
	catch (avk::runtime_error&) {}
}


