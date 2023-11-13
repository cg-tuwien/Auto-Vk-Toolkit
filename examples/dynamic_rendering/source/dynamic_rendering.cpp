#include "avk/attachment.hpp"
#include "avk/command_buffer.hpp"
#include "avk/commands.hpp"
#include "avk/layout.hpp"
#include "avk/memory_access.hpp"
#include "avk/pipeline_stage.hpp"
#include "imgui.h"

#include "configure_and_compose.hpp"
#include "imgui_manager.hpp"
#include "invokee.hpp"
#include "sequential_invoker.hpp"

#include "context_vulkan.hpp"
#include <string>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_structs.hpp>

class dynamic_rendering_app : public avk::invokee
{
public: // v== avk::invokee overrides which will be invoked by the framework ==v
	dynamic_rendering_app(avk::queue& aQueue)
		: mQueue{ &aQueue }
		, mXSplit{0.5f}
		, mYSplit{0.5f}
		, mBorderThickness{2u}
		, mFullscreenViewport{true}
		, mClearColors{{
			{0.25f, 0.25f, 0.25f, 0.25f},
			{0.50f, 0.50f, 0.50f, 0.50f},
			{0.75f, 0.75f, 0.75f, 0.75f},
			{1.00f, 1.00f, 1.00f, 1.00f},
		}}
	{ }

	void initialize() override
	{
		const auto r = avk::context().main_window()->resolution();
		auto colorAttachmentDescription = avk::dynamic_rendering_attachment::declare_for(avk::context().main_window()->current_image_view_reference());
		
		// Create graphics pipeline for rasterization with the required configuration:
		mPipeline = avk::context().create_graphics_pipeline_for(
			avk::vertex_shader("shaders/screen_space_tri.vert"),                // Add a vertex shader
			avk::fragment_shader("shaders/color.frag"),                         // Add a fragment shader
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(         
				avk::context().main_window()->backbuffer_reference_at_index(0)) // Get scissor and viewport settings from current window backbuffer
				.enable_dynamic_viewport(),                                     // But also enable dynamic viewport
			avk::cfg::dynamic_rendering::enabled,
			colorAttachmentDescription
		);

		// Get hold of the "ImGui Manager" and add a callback that draws UI elements:
		auto imguiManager = avk::current_composition()->element_by_type<avk::imgui_manager>();
		if (nullptr != imguiManager) {
			imguiManager->add_callback([this, r](){
		        ImGui::Begin("Info & Settings");
				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
				for(uint32_t i = 0; i < mClearColors.size(); i++)
				{
					ImGui::ColorEdit4((std::string("Clear color renderpass ") + std::to_string(i)).c_str(), mClearColors.at(i).data());
				}
				const float minXSize = (1.0f + mBorderThickness) / static_cast<float>(r.x);
				const float minYSize = (1.0f + mBorderThickness) / static_cast<float>(r.y);
				ImGui::SliderFloat("Renderpass x split", &mXSplit, minXSize, 1.0f - minXSize, "%.3f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderFloat("Renderpass y split", &mYSplit, minYSize, 1.0f - minYSize, "%.3f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::SliderInt("Border Thickness", reinterpret_cast<int32_t*>(&mBorderThickness), 0, 10);
				ImGui::Checkbox("Fullscreen viewport", &mFullscreenViewport);
				ImGui::End();
			});
		}
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
		auto inFlightIndex = mainWnd->in_flight_index_for_frame();

		// Get a command pool to allocate command buffers from:
		auto& commandPool = avk::context().get_command_pool_for_single_use_command_buffers(*mQueue);

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();
		
		// Create a single command buffer:
		auto cmdBfr = commandPool->alloc_command_buffers(1u, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		
		avk::image_view swapchainImageView = mainWnd->current_image_view();
		avk::image_t const & swapchainImage = mainWnd->current_image_reference(); 
		avk::attachment colorAttachment = avk::attachment::declare_for(swapchainImageView.as_reference(), avk::on_load::clear, avk::usage::color(0), avk::on_store::store);

		std::vector<avk::recorded_commands_t> renderCommands = {};
		// First we transition the swapchain image into color attachment format
		render_commands.emplace_back(
			avk::sync::image_memory_barrier(swapchainImage,
				// source stage none because it is handled by imageAvailable semaphore
				avk::stage::none >> avk::stage::color_attachment_output,
				avk::access::none >> avk::access::color_attachment_write
				// We don't care about the previous contents of the swapchain image (they probably not even defined!)
			).with_layout_transition(avk::layout::undefined >> avk::layout::color_attachment_optimal)
		);

		auto const windowResolution = mainWnd->resolution();
		// Set the dynamic viewport only once at the start if we want shared fullscreen viewport for each renderpass
		if(mFullscreenViewport) {
			auto const currentViewport = vk::Viewport(0.0f, 0.0f, windowResolution.x, windowResolution.y);
			render_commands.emplace_back(avk::command::custom_commands([=](avk::command_buffer_t& cb) {
				cb.handle().setViewport(0, 1, &currentViewport);
			}));
		} 

		// We render the image in four renderpasses - each renderpass has it's own clear value as well as offset and extent.
		// Note that the renderpass extent acts as "clip" on top of the pipeline viewport (which is set to be the entire window).
		// Because of this we still get a single triangle just drawn in four parts, if we instead wanted to see four triangles we would
		// need to set the pipeline viewport to be 
		for(uint32_t renderpassIdx = 0; renderpassIdx < 4; renderpassIdx++)
		{
			float xCoord = renderpassIdx % 2;
			float yCoord = renderpassIdx / 2u; //NOLINT(bugprone-integer-division) Standard way of unpacking 2d y coordinate from 1d encoding
			vk::Offset2D const currOffset{
				static_cast<int32_t>(std::min(static_cast<uint32_t>(xCoord * mXSplit * windowResolution.x + xCoord * mBorderThickness), windowResolution.x)),
				static_cast<int32_t>(std::min(static_cast<uint32_t>(yCoord * mYSplit * windowResolution.y + yCoord * mBorderThickness), windowResolution.y))
			};
			vk::Extent2D const currExtent{
				static_cast<uint32_t>(std::max(static_cast<int32_t>(std::abs(xCoord - mXSplit) * windowResolution.x - mBorderThickness), 0)),
				static_cast<uint32_t>(std::max(static_cast<int32_t>(std::abs(yCoord - mYSplit) * windowResolution.y - mBorderThickness), 0))
			};
			colorAttachment.set_clear_color(mClearColors.at(renderpassIdx));
			// Set the dynamic viewport to be equal to each of the renderpass extent and offsets if don't want shared fullscreen viewport for all of them
			render_commands.emplace_back(avk::command::begin_dynamic_rendering({colorAttachment}, {swapchainImageView}, currOffset, currExtent));
			render_commands.emplace_back(avk::command::bind_pipeline(mPipeline.as_reference()));
			if(!mFullscreenViewport)
			{
				auto const currentViewport = vk::Viewport(currOffset.x, currOffset.y, currExtent.width, currExtent.height);
				render_commands.emplace_back(avk::command::custom_commands([=](avk::command_buffer_t& cb) {
					cb.handle().setViewport(0, 1, &currentViewport);
				}));
			}
			render_commands.emplace_back(avk::command::draw(3u, 1u, 0u, 0u));
			render_commands.emplace_back(avk::command::end_dynamic_rendering());
		}

		avk::context().record(render_commands)
		.into_command_buffer(cmdBfr[0])
		.then_submit_to(*mQueue)
		.waiting_for(imageAvailableSemaphore >> avk::stage::color_attachment_output)
		.submit();

		avk::context().main_window()->handle_lifetime(std::move(cmdBfr[0]));
	}


private: // v== Member variables ==v

	avk::queue* mQueue;
	avk::graphics_pipeline mPipeline;
	float mXSplit;
	float mYSplit;
	uint32_t mBorderThickness;
	bool mFullscreenViewport;
	std::array<std::array<float, 4>, 4> mClearColors;

}; // vertex_buffers_app

int main() // <== Starting point ==
{
	int result = EXIT_FAILURE;
	try {
		// Create a window and open it
		auto mainWnd = avk::context().create_window("Dynamic rendering");
		mainWnd->set_resolution({ 1920, 1080 });
		// mainWnd->enable_resizing(true);
		mainWnd->set_presentaton_mode(avk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = avk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->set_queue_family_ownership(singleQueue.family_index());
		mainWnd->set_present_queue(singleQueue);
		
		// Create an instance of our main "invokee" which contains all the functionality:
		auto app = dynamic_rendering_app(singleQueue);
		// Create another invokee for drawing the UI with ImGui
		auto ui = avk::imgui_manager(singleQueue);

		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			avk::application_name("Auto-Vk-Toolkit Example: Dynamic rendering"),
			[](avk::validation_layers& config) {
				config.enable_feature(vk::ValidationFeatureEnableEXT::eSynchronizationValidation);
			},
			avk::required_device_extensions()
			.add_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME),
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
