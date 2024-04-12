#include "auto_vk_toolkit.hpp"
#include "imgui.h"

#include "configure_and_compose.hpp"
#include "imgui_manager.hpp"
#include "invokee.hpp"
#include "sequential_invoker.hpp"

class vertex_buffers_app : public avk::invokee
{
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

public:
	vertex_buffers_app(avk::queue& aQueue)
		: mQueue{ &aQueue }
		, mAdditionalTranslationY{ 0.0f }
		, mRotationSpeed{ 1.0f }
	{ }

	void initialize() override
	{
		// Create graphics pipeline for rasterization with the required configuration:
		mPipeline = avk::context().create_graphics_pipeline_for(
			avk::from_buffer_binding(0) -> stream_per_vertex(&Vertex::pos)   -> to_location(0),	// Describe the position vertex attribute
			avk::from_buffer_binding(0) -> stream_per_vertex(&Vertex::color) -> to_location(1), // Describe the color vertex attribute
			avk::vertex_shader("shaders/passthrough.vert"),										// Add a vertex shader
			avk::fragment_shader("shaders/color.frag"),											// Add a fragment shader
			avk::cfg::front_face::define_front_faces_to_be_clockwise(),							// Front faces are in clockwise order
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)), // Align viewport with main window's resolution
			avk::context().main_window()->get_renderpass()
		);

		// Create vertex buffers --- namely one for each frame in flight.
		// We create multiple vertex buffers because we'll update the data every frame and frames run concurrently.
		// However, do not upload vertices yet. we'll do that in the render() method.
		auto numFramesInFlight = avk::context().main_window()->number_of_frames_in_flight();
		for (int i = 0; i < numFramesInFlight; ++i) {
			mVertexBuffers.emplace_back(
				avk::context().create_buffer(
					avk::memory_usage::device, {},							// Create the buffer on the device, i.e. in GPU memory, (no additional usage flags).
					avk::vertex_buffer_meta::create_from_data(mVertexData)		// Infer meta data from the given buffer.
				)
			);
		}
		
		// Create index buffer. Upload data already since we won't change it ever
		// (hence the usage of avk::create_and_fill instead of just avk::create)
		mIndexBuffer = avk::context().create_buffer(
			avk::memory_usage::device, {},										// Also this buffer should "live" in GPU memory
			avk::index_buffer_meta::create_from_data(mIndices)					// Pass/create meta data about the indices
		);

		// Fill it with data already here, in initialize(), because this buffer will stay constant forever:
		auto fence = avk::context().record_and_submit_with_fence({ mIndexBuffer->fill(mIndices.data(), 0) }, *mQueue);
		// Wait with a fence until the data transfer has completed:
		fence->wait_until_signalled();

		// Get hold of the "ImGui Manager" and add a callback that draws UI elements:
		auto imguiManager = avk::current_composition()->element_by_type<avk::imgui_manager>();
		if (nullptr != imguiManager) {
			imguiManager->add_callback([this](){
		        ImGui::Begin("Info & Settings");
				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
				ImGui::SliderFloat("Translation", &mAdditionalTranslationY, -1.0f, 1.0f);
				ImGui::InputFloat("Rotation Speed", &mRotationSpeed, 0.1f, 1.0f);
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
		// Modify our vertex data according to our rotation animation and upload this frame's vertex data:
		auto rotAngle = glm::radians(90.0f) * avk::time().time_since_start() * mRotationSpeed;
		auto rotMatrix = glm::rotate(rotAngle, glm::vec3(0.f, 1.f, 0.f));
		auto translateZ = glm::translate(glm::vec3{ 0.0f, 0.0f, -0.5f });
		auto invTranslZ = glm::inverse(translateZ);
		auto translateY = glm::translate(glm::vec3{ 0.0f, -mAdditionalTranslationY, 0.0f });

		// Store modified vertices in vertexDataCurrentFrame:
		std::vector<Vertex> vertexDataCurrentFrame;
		for (const Vertex& vtx : mVertexData) {
			glm::vec4 origPos{ vtx.pos, 1.0f };
			vertexDataCurrentFrame.push_back({
				glm::vec3(translateY * invTranslZ * rotMatrix * translateZ * origPos),
				vtx.color
			});
		}

		// Note: Updating this data in update() would also be perfectly fine when using a varying_update_timer.
		//		 However, when using a fixed_update_timer --- where update() and render() might not be called
		//		 in sync --- it can make a difference.
		//		 Since the buffer-updates here are solely rendering-relevant and do not depend on other aspects
		//		 like e.g. input or physics simulation, it makes most sense to perform them in render(). This
		//		 will ensure correct and smooth rendering regardless of the timer used.

		auto mainWnd = avk::context().main_window();
		auto inFlightIndex = mainWnd->in_flight_index_for_frame();

		// Get a command pool to allocate command buffers from:
		auto& commandPool = avk::context().get_command_pool_for_single_use_command_buffers(*mQueue);

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();
		
		// Create a command buffer and render into the *current* swap chain image:
		auto cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		
		avk::context().record({
				// Fill the vertex buffer that corresponds to this the current inFlightIndex:
				mVertexBuffers[inFlightIndex]->fill(vertexDataCurrentFrame.data(), 0),

				// Install a barrier to synchronize the buffer copy with the subsequent render pass:
				avk::sync::buffer_memory_barrier(mVertexBuffers[inFlightIndex].as_reference(), 
					avk::stage::copy				>> avk::stage::vertex_attribute_input,
					avk::access::transfer_write		>> avk::access::vertex_attribute_read
				),

				// Note: We could also let the framework infer the stages automatically as follows:
				// avk::sync::buffer_memory_barrier(mVertexBuffers[inFlightIndex].as_reference(), avk::stage::auto_stage >> avk::stage::auto_stage, avk::access:: auto_access >> avk::access::auto_access),
				// This ^ will lead to correct synchronization, but it might lead to suboptimal stages or access masks -- in particular if
				// render passes are involved in automatic determination, because we don't know where exactly the buffer is first used.

				// Hint: The following would lead to the same barrier as the one above:
				// avk::sync::buffer_memory_barrier(mVertexBuffers[inFlightIndex].as_reference(), avk::stage::auto_stage >> avk::stage::vertex_attribute_input, avk::access:: auto_access >> avk::access::vertex_attribute_read),

				// Begin and end one renderpass:
				avk::command::render_pass(mPipeline->renderpass_reference(), avk::context().main_window()->current_backbuffer_reference(), {
					// And within, bind a pipeline and perform an indexed draw call:
					avk::command::bind_pipeline(mPipeline.as_reference()),
					avk::command::draw_indexed(mIndexBuffer.as_reference(), mVertexBuffers[inFlightIndex].as_reference())
				})
			})
			.into_command_buffer(cmdBfr)
			.then_submit_to(*mQueue)
			// Do not start to render before the image has become available:
			.waiting_for(imageAvailableSemaphore >> avk::stage::color_attachment_output)
			.submit();

		// Use a convenience function of avk::window to take care of the command buffer's lifetime:
		// It will get deleted in the future after #concurrent-frames have passed by.
		avk::context().main_window()->handle_lifetime(std::move(cmdBfr));
	}


private: // v== Member variables ==v
	
	avk::queue* mQueue;
	std::vector<avk::buffer> mVertexBuffers;
	avk::buffer mIndexBuffer;
	avk::graphics_pipeline mPipeline;
	float mAdditionalTranslationY;
	float mRotationSpeed;

}; // vertex_buffers_app

int main() // <== Starting point ==
{
	int result = EXIT_FAILURE;
	try {
		// Create a window and open it
		auto mainWnd = avk::context().create_window("Vertex Buffers");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->set_presentaton_mode(avk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = avk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->set_queue_family_ownership(singleQueue.family_index());
		mainWnd->set_present_queue(singleQueue);
		
		// Create an instance of our main "invokee" which contains all the functionality:
		auto app = vertex_buffers_app(singleQueue);
		// Create another invokee for drawing the UI with ImGui
		auto ui = avk::imgui_manager(singleQueue);

		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			avk::application_name("Auto-Vk-Toolkit Example: Vertex Buffers"),
			[](avk::validation_layers& config) {
				//config.enable_feature(vk::ValidationFeatureEnableEXT::eSynchronizationValidation);
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
