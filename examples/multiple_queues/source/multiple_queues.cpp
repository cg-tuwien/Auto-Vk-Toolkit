#include "imgui.h"

#include "configure_and_compose.hpp"
#include "imgui_manager.hpp"
#include "invokee.hpp"
#include "sequential_invoker.hpp"

class multiple_queues_app : public avk::invokee
{
	// Define a struct for our vertex input data:
	struct Vertex
	{
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
	multiple_queues_app(std::array<avk::queue*, 2> aTransferQueues, avk::queue* aGraphicsQueue)
		: mTransferQueues{ aTransferQueues }, mGraphicsQueue{ aGraphicsQueue }
	{ }

	void initialize() override
	{
		using namespace avk;

		// Create graphics pipeline for rasterization with the required configuration:
		mPipeline = avk::context().create_graphics_pipeline_for(
			from_buffer_binding(0) -> stream_per_vertex(&Vertex::pos)   -> to_location(0),	// Describe the position vertex attribute
			from_buffer_binding(0) -> stream_per_vertex(&Vertex::color) -> to_location(1), // Describe the color vertex attribute
			vertex_shader("shaders/passthrough.vert"),										// Add a vertex shader
			fragment_shader("shaders/color.frag"),											// Add a fragment shader
			cfg::front_face::define_front_faces_to_be_clockwise(),							// Front faces are in clockwise order
			cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)), // Align viewport with main window's resolution
			avk::context().main_window()->get_renderpass()
		);

		// Create vertex buffers --- namely one for each frame in flight.
		// We create multiple vertex buffers because we'll update the data every frame and frames run concurrently.
		// However, do not upload vertices yet. we'll do that in the render() method.
		auto numFramesInFlight = avk::context().main_window()->number_of_frames_in_flight();
		for (int i = 0; i < numFramesInFlight; ++i) {
			auto& vertexBuffersPerFrameInFlight = mVertexBuffers.emplace_back();
			for (int j = 0; j < 2; ++j) {
				vertexBuffersPerFrameInFlight[j] = avk::context().create_buffer(
					memory_usage::device, {},							// Create the buffer on the device, i.e. in GPU memory, (no additional usage flags).
					vertex_buffer_meta::create_from_data(mVertexData)		// Infer meta data from the given buffer.
				);
			}
		}
		
		// Create an index buffer and upload it:
		mIndexBuffer[0] = avk::context().create_buffer(memory_usage::device, {}, index_buffer_meta::create_from_data(mIndices));
		avk::context().record_and_submit_with_fence({ mIndexBuffer[0]->fill(mIndices.data(), 0) }, *mTransferQueues[0])->wait_until_signalled();
		// Let's see what to do with the other tranfer queues:
		for (int j = 1; j < 2; ++j) {
			// If all the transfer queues are from the same queue family, they can all share the same index buffer -- if not, then they can't:
			if (mTransferQueues[0]->family_index() == mTransferQueues[j]->family_index()) {
				mIndexBuffer[j] = mIndexBuffer[0];
				continue;
			}
			mIndexBuffer[j] = avk::context().create_buffer(memory_usage::device, {}, index_buffer_meta::create_from_data(mIndices));

			// Fill and then transfer ownership to the graphics queue:
			avk::context().record_and_submit_with_fence({ 
				mIndexBuffer[j]->fill(mIndices.data(), 0),
				// A queue family ownership transfer consists of two distinct parts (as per specification 7.7.4. Queue Family Ownership Transfer): 
				//  1. Release exclusive ownership from the source queue family
				sync::buffer_memory_barrier(mIndexBuffer[j].as_reference(), stage::auto_stage + access::auto_access >> stage::none + access::none)
					.with_queue_family_ownership_transfer(mTransferQueues[j]->family_index(), mGraphicsQueue->family_index())
			}, *mTransferQueues[j])->wait_until_signalled();
			    //  2. Acquire exclusive ownership for the destination queue family
			avk::context().record_and_submit_with_fence({
				sync::buffer_memory_barrier(mIndexBuffer[j].as_reference(), stage::none + access::none >> stage::auto_stage + access::auto_access)
					.with_queue_family_ownership_transfer(mTransferQueues[j]->family_index(), mGraphicsQueue->family_index())
			}, *mGraphicsQueue)->wait_until_signalled();
		}

		// Get hold of the "ImGui Manager" and add a callback that draws UI elements:
		auto imguiManager = avk::current_composition()->element_by_type<avk::imgui_manager>();
		if (nullptr != imguiManager) {
			imguiManager->add_callback([this](){
		        ImGui::Begin("Info & Settings");
				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
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
		using namespace avk;

		// For the right vertex buffer, ...
		auto mainWnd = avk::context().main_window();
		auto inFlightIndex = mainWnd->in_flight_index_for_frame();
		
		// Prepare three semaphores to sync the three transfer queues with the graphics queue:
		std::array<semaphore, 2> vertexBufferFillSemaphores;

		// Translate all differently and submit to different queues:
		for (int j = 0; j < 2; ++j) {
			// Modify our vertex data according to our rotation animation and upload this frame's vertex data:
			auto rotAngle = glm::radians(90.0f) * avk::time().time_since_start() * static_cast<float>(j + 1);
			auto rotMatrix = glm::rotate(rotAngle, glm::vec3(0.f, 1.f, 0.f));
			auto translateZ = glm::translate(glm::vec3{ 0.0f, 0.0f, -0.5f });
			auto invTranslZ = glm::inverse(translateZ);
			auto translateX = glm::translate(glm::vec3{ static_cast<float>(j) - 0.5f, 0.0f, 0.0f });

			// Compute modified vertices:
			std::vector<Vertex> modifiedVertices;
			for (const Vertex& vtx : mVertexData) {
				modifiedVertices.push_back({glm::vec3(translateX * invTranslZ * rotMatrix * translateZ * glm::vec4{ vtx.pos, 1.0f }), vtx.color });
			}

			// Transfer all to different queues:
			const auto& vertexBuffer = mVertexBuffers[inFlightIndex][j].as_reference();
			vertexBufferFillSemaphores[j] = avk::context().record_and_submit_with_semaphore(command::gather( // Need semaphores for queue -> queue synchronization
					command::conditional(
						[frameId = avk::context().main_window()->current_frame()]() { // Skip ownership transfer for the very first frame, because in the very first frame, there is no owner yet
							return frameId >= 3; // Because we have three concurrent frames
						},
						[&]() {
							// A queue family ownership transfer consists of two distinct parts (as per specification 7.7.4. Queue Family Ownership Transfer): 
							//  2. Acquire exclusive ownership for the destination queue family
							return sync::buffer_memory_barrier(vertexBuffer, stage::none + access::none >> stage::auto_stage + access::auto_access)
								.with_queue_family_ownership_transfer(mGraphicsQueue->family_index(), mTransferQueues[j]->family_index());
						}
					),

					// Perform copy command:
					vertexBuffer.fill(modifiedVertices.data(), 0),

					// A queue family ownership transfer consists of two distinct parts (as per specification 7.7.4. Queue Family Ownership Transfer): 
					//  1. Release exclusive ownership from the source queue family
					sync::buffer_memory_barrier(vertexBuffer, stage::auto_stage + access::auto_access >> stage::none + access::none)
						.with_queue_family_ownership_transfer(mTransferQueues[j]->family_index(), mGraphicsQueue->family_index())
				), *mTransferQueues[j], stage::auto_stage // Let the framework determine the (source) stage after which the semaphore can be signaled (will be stage::copy due to buffer_t::fill)
			);

		}
		

		// Get a command pool to allocate command buffers from:
		auto& commandPool = avk::context().get_command_pool_for_single_use_command_buffers(*mGraphicsQueue);

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();
		
		// Create a command buffer and render into the *current* swap chain image:
		auto cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		avk::context().record({
				// A queue family ownership transfer consists of two distinct parts (as per specification 7.7.4. Queue Family Ownership Transfer): 
				//  2. Acquire exclusive ownership for the destination queue family
				sync::buffer_memory_barrier(mVertexBuffers[inFlightIndex][0].as_reference(), stage::none + access::none >> stage::vertex_attribute_input + access::vertex_attribute_read)
					.with_queue_family_ownership_transfer(mTransferQueues[0]->family_index(), mGraphicsQueue->family_index()),
				sync::buffer_memory_barrier(mVertexBuffers[inFlightIndex][1].as_reference(), stage::none + access::none >> stage::vertex_attribute_input + access::vertex_attribute_read)
					.with_queue_family_ownership_transfer(mTransferQueues[1]->family_index(), mGraphicsQueue->family_index()),

				command::render_pass(mPipeline->renderpass_reference(), avk::context().main_window()->current_backbuffer_reference(), {
					// And within, bind a pipeline and perform an indexed draw call:
					command::bind_pipeline(mPipeline.as_reference()),
					// Two draw calls with all the buffer ownerships now transferred to the graphics queue:
					command::draw_indexed(mIndexBuffer[0].as_reference(), mVertexBuffers[inFlightIndex][0].as_reference()),
					command::draw_indexed(mIndexBuffer[1].as_reference(), mVertexBuffers[inFlightIndex][1].as_reference()),
				}),

				// After we are done, we've gotta return the ownership of the buffers back to the respective transfer queue:
				// A queue family ownership transfer consists of two distinct parts (as per specification 7.7.4. Queue Family Ownership Transfer): 
				//  1. Release exclusive ownership from the source queue family
				sync::buffer_memory_barrier(mVertexBuffers[inFlightIndex][0].as_reference(), stage::vertex_attribute_input + access::vertex_attribute_read >> stage::none + access::none)
					.with_queue_family_ownership_transfer(mGraphicsQueue->family_index(), mTransferQueues[0]->family_index()),
				sync::buffer_memory_barrier(mVertexBuffers[inFlightIndex][1].as_reference(), stage::vertex_attribute_input + access::vertex_attribute_read >> stage::none + access::none)
					.with_queue_family_ownership_transfer(mGraphicsQueue->family_index(), mTransferQueues[1]->family_index()),
			})
			.into_command_buffer(cmdBfr)
			.then_submit_to(*mGraphicsQueue)
			// Do not start to render before the image has become available:
			.waiting_for(imageAvailableSemaphore >> stage::color_attachment_output)
			// Also wait for the data transfer into the vertex buffer has completed:
			.waiting_for(vertexBufferFillSemaphores[0] >> stage::vertex_attribute_input)
			.waiting_for(vertexBufferFillSemaphores[1] >> stage::vertex_attribute_input)
			.submit();

		// Let the command buffer handle the lifetimes of all the semaphores:
		cmdBfr->handle_lifetime_of(std::move(vertexBufferFillSemaphores[0]));
		cmdBfr->handle_lifetime_of(std::move(vertexBufferFillSemaphores[1]));

		// Use a convenience function of avk::window to take care of the command buffer's lifetime:
		// It will get deleted in the future after #concurrent-frames have passed by.
		avk::context().main_window()->handle_lifetime(std::move(cmdBfr));
	}


private: // v== Member variables ==v
	
	std::array<avk::queue*, 2> mTransferQueues;
	avk::queue* mGraphicsQueue;
	std::vector<std::array<avk::buffer, 2>> mVertexBuffers;
	std::array<avk::buffer, 2> mIndexBuffer;
	avk::graphics_pipeline mPipeline;

}; // multiple_queues_app

int main() // <== Starting point ==
{
	int result = EXIT_FAILURE;
	try {
		// Create a window and open it
		auto mainWnd = avk::context().create_window("Multiple Queues");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->set_presentaton_mode(avk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		// Create two transfer queues and one graphics queue:
		std::array<avk::queue*, 2> transferQueues = {
			&avk::context().create_queue(vk::QueueFlagBits::eTransfer, avk::queue_selection_preference::specialized_queue),
			&avk::context().create_queue(vk::QueueFlagBits::eTransfer, avk::queue_selection_preference::specialized_queue)
		};
		avk::queue* graphicsQueue = &avk::context().create_queue(vk::QueueFlagBits::eGraphics, avk::queue_selection_preference::specialized_queue, mainWnd);
		// Only the graphics queue shall own the swapchain images, it is the queue which is used for the present call:
		mainWnd->set_queue_family_ownership(graphicsQueue->family_index());
		mainWnd->set_present_queue(*graphicsQueue);
		
		// Create an instance of our main "invokee" which contains all the functionality:
		auto app = multiple_queues_app(transferQueues, graphicsQueue);
		// Create another invokee for drawing the UI with ImGui
		auto ui = avk::imgui_manager(*graphicsQueue);

		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			avk::application_name("Auto-Vk-Toolkit Example: Multiple Queues"),
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
