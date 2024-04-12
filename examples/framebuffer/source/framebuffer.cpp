#include "imgui.h"

#include "configure_and_compose.hpp"
#include "imgui_manager.hpp"
#include "invokee.hpp"
#include "sequential_invoker.hpp"

#include "context_vulkan.hpp"

class framebuffer_app : public avk::invokee
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

public: // v== xk::invokee overrides which will be invoked by the framework ==v
	framebuffer_app(avk::queue& aQueue)
		: mQueue{ &aQueue }
		, mSelectedAttachmentToCopy{ 0 }
		, mAdditionalTranslationY{ 0.0f }
		, mRotationSpeed{ 1.0f }
	{ }

	void initialize() override
	{
		const auto r = avk::context().main_window()->resolution();
		auto colorAttachment = avk::context().create_image_view(avk::context().create_image(r.x, r.y, vk::Format::eR8G8B8A8Unorm, 1, avk::memory_usage::device, avk::image_usage::general_color_attachment));
		auto depthAttachment = avk::context().create_image_view(avk::context().create_image(r.x, r.y, vk::Format::eD32Sfloat, 1, avk::memory_usage::device, avk::image_usage::general_depth_stencil_attachment));
		auto colorAttachmentDescription = avk::attachment::declare_for(colorAttachment.as_reference(), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::color(0)     , avk::on_store::store);
		auto depthAttachmentDescription = avk::attachment::declare_for(depthAttachment.as_reference(), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::depth_stencil, avk::on_store::store);

		mOneFramebuffer = avk::context().create_framebuffer(
			{ colorAttachmentDescription, depthAttachmentDescription }, // Attachment declarations can just be copied => use initializer_list.
			avk::make_vector( colorAttachment, depthAttachment )
		);
		
		// Create graphics pipeline for rasterization with the required configuration:
		mPipeline = avk::context().create_graphics_pipeline_for(
			avk::from_buffer_binding(0) -> stream_per_vertex(&Vertex::pos)   -> to_location(0),	// Describe the position vertex attribute
			avk::from_buffer_binding(0) -> stream_per_vertex(&Vertex::color) -> to_location(1), // Describe the color vertex attribute
			avk::vertex_shader("shaders/passthrough.vert"),                                   // Add a vertex shader
			avk::fragment_shader("shaders/color.frag"),                                       // Add a fragment shader
			avk::cfg::front_face::define_front_faces_to_be_clockwise(),							// Front faces are in clockwise order
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)),	// Align viewport with main window's resolution
			colorAttachmentDescription,
			depthAttachmentDescription
		);

		// Create vertex buffers --- namely one for each frame in flight.
		// We create multiple vertex buffers because we'll update the data every frame and frames run concurrently.
		// However, do not upload vertices yet. we'll do that in the render() method.
		auto numFramesInFlight = avk::context().main_window()->number_of_frames_in_flight();
		for (int i = 0; i < numFramesInFlight; ++i) {
			mVertexBuffers.emplace_back(
				avk::context().create_buffer(
					avk::memory_usage::device, {},								// Create the buffer on the device, i.e. in GPU memory, (no additional usage flags).
					avk::vertex_buffer_meta::create_from_data(mVertexData)		// Infer meta data from the given buffer.
				)
			);
		}
		
		// Create index buffer. Upload data already since we won't change it ever
		// (hence the usage of avk::create_and_fill instead of just avk::create)
		mIndexBuffer = avk::context().create_buffer(
			avk::memory_usage::device, {},                    // Also this buffer should "live" in GPU memory
			avk::index_buffer_meta::create_from_data(mIndices)  // Pass/create meta data about the indices
		);

		// Fill it with data already here, in initialize(), because this buffer will stay constant forever.

		// Use a convenience method to record commands, submit to a queue, and getting a fence back:
		auto fence = avk::context().record_and_submit_with_fence({ mIndexBuffer->fill(mIndices.data(), 0) }, *mQueue);
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

				ImGui::Separator();
				ImGui::Text("Which attachment to copy/blit:");
				static const char* optionsAttachment[] = {"Color attachment at index 0", "Depth attachment at index 1"};
				ImGui::Combo("##att", &mSelectedAttachmentToCopy, optionsAttachment, 2);
				ImGui::Text("Which transfer operation to use:");
				static const char* optionsOperation[] = {"Copy", "Blit"};
				ImGui::Combo("##op", &mUseCopyOrBlit, optionsOperation, 2);
		        ImGui::End();
			});
		}
	}

	void update() override
	{
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
		//		 However, when using a fixed_update_timer, where update() and render() might not be called
		//		 in sync, it can make a difference.
		//		 Since the buffer-updates here are solely rendering-relevant and do not depend on other aspects
		//		 like e.g. input or physics simulation, it makes most sense to perform them in render(). This
		//		 will ensure correct and smooth rendering regardless of the timer used.

		// For the current frame's vertex buffer, ...
		auto mainWnd = avk::context().main_window();
		auto inFlightIndex = mainWnd->in_flight_index_for_frame();

		// ... update its vertex data, then get a semaphore which signals as soon as the operation has completed:
		auto vertexBufferFillSemaphore = avk::context().record_and_submit_with_semaphore({
				mVertexBuffers[inFlightIndex]->fill(vertexDataCurrentFrame.data(), 0)
			}, 
			*mQueue,
			avk::stage::auto_stage // Let the framework determine the (source) stage after which the semaphore can be signaled (will be stage::copy due to buffer_t::fill)
		);

		// Get a command pool to allocate command buffers from:
		auto& commandPool = avk::context().get_command_pool_for_single_use_command_buffers(*mQueue);

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();
		
		// Create two command buffer:
		auto cmdBfrs = commandPool->alloc_command_buffers(2u, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		
		// Create a new semaphore to establish a dependency between different batches of work:
		auto renderCompleteSemaphore = avk::context().create_semaphore();

		avk::context().record({
				// Begin and end one renderpass:
				avk::command::render_pass(mPipeline->renderpass_reference(), mOneFramebuffer.as_reference(), {
					// And within, bind a pipeline and perform an indexed draw call:
					avk::command::bind_pipeline(mPipeline.as_reference()),
					avk::command::draw_indexed(mIndexBuffer.as_reference(), mVertexBuffers[inFlightIndex].as_reference())
				})
			})
			.into_command_buffer(cmdBfrs[0])
			.then_submit_to(*mQueue)
			// Do not start to render before the image has become available:
			.waiting_for(imageAvailableSemaphore >> avk::stage::color_attachment_output)
			// Also wait for the data transfer into the vertex buffer has completed:
			.waiting_for(vertexBufferFillSemaphore >> avk::stage::vertex_attribute_input)
			// Signal the following semaphore upon completion, only to wait on it in the next submitted batch of work (see below)
			.signaling_upon_completion(avk::stage::color_attachment_output >> renderCompleteSemaphore)
			.submit();

		// Let the command buffer handle the semaphore lifetimes:
		cmdBfrs[0]->handle_lifetime_of(std::move(vertexBufferFillSemaphore));

		// Note: Using a semaphore here is okay, but it is a bit heavy-weight.
		//       A more low-weight alternative would be to use a memory barrier.

		auto sourceImageLayout = 0 == mSelectedAttachmentToCopy
			? avk::layout::color_attachment_optimal
			: avk::layout::depth_attachment_optimal;

		avk::context().record(avk::command::gather( // Use command::gather here instead of passing a std::vector here.
			                                        // This allows us to use command::conditional further down.

				// Transition the layouts before performing the transfer operation:
				avk::sync::image_memory_barrier(mOneFramebuffer->image_at(mSelectedAttachmentToCopy),
					// None here, because we're synchronizing with a semaphore
					avk::stage::none  >> avk::stage::copy | avk::stage::blit,
					avk::access::none >> avk::access::transfer_read
				).with_layout_transition(sourceImageLayout >> avk::layout::transfer_src),
				avk::sync::image_memory_barrier(mainWnd->current_backbuffer()->image_at(0),
					avk::stage::none  >> avk::stage::copy | avk::stage::blit,
					avk::access::none >> avk::access::transfer_write
				).with_layout_transition(avk::layout::undefined >> avk::layout::transfer_dst),

				// Perform the transfer operation:
				avk::command::conditional(
					[&]() { return 0 == mUseCopyOrBlit;  },
					[&]() {
						return avk::copy_image_to_another(
							mOneFramebuffer->image_at(mSelectedAttachmentToCopy), avk::layout::transfer_src,
							mainWnd->current_backbuffer()->image_at(0), avk::layout::transfer_dst
						);
					},
					[&]() { 
						return avk::blit_image(
							mOneFramebuffer->image_at(mSelectedAttachmentToCopy), avk::layout::transfer_src,
							mainWnd->current_backbuffer()->image_at(0), avk::layout::transfer_dst
						);
					}
				),

				// Transition the layouts back:
				avk::sync::image_memory_barrier(mOneFramebuffer->image_at(mSelectedAttachmentToCopy),
					avk::stage::copy | avk::stage::blit            >> avk::stage::none,
					avk::access::transfer_read                     >> avk::access::none
				).with_layout_transition(avk::layout::transfer_src >> sourceImageLayout), // Restore layout
				avk::sync::image_memory_barrier(mainWnd->current_backbuffer()->image_at(0),
					avk::stage::copy | avk::stage::blit            >> avk::stage::color_attachment_output,
					avk::access::transfer_write                    >> avk::access::color_attachment_write
				).with_layout_transition(avk::layout::transfer_dst >> avk::layout::color_attachment_optimal)

			))
			.into_command_buffer(cmdBfrs[1])
			.then_submit_to(*mQueue)
			.waiting_for(renderCompleteSemaphore >> (avk::stage::copy | avk::stage::blit))
			.submit();

		// Let the latter command buffer handle the former batch's renderCompleteSemaphore lifetime:
		cmdBfrs[1]->handle_lifetime_of(std::move(renderCompleteSemaphore));

		// Use a convenience function of avk::window to take care of the command buffers lifetimes:
		// They will get deleted in the future after #concurrent-frames have passed by.
		avk::context().main_window()->handle_lifetime(std::move(cmdBfrs[0]));
		avk::context().main_window()->handle_lifetime(std::move(cmdBfrs[1]));
	}


private: // v== Member variables ==v

	avk::queue* mQueue;
	int mSelectedAttachmentToCopy;
	int mUseCopyOrBlit;
	avk::framebuffer mOneFramebuffer;
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
		auto mainWnd = avk::context().create_window("Framebuffers");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->set_presentaton_mode(avk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = avk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->set_queue_family_ownership(singleQueue.family_index());
		mainWnd->set_present_queue(singleQueue);
		
		// Create an instance of our main "invokee" which contains all the functionality:
		auto app = framebuffer_app(singleQueue);
		// Create another invokee for drawing the UI with ImGui
		auto ui = avk::imgui_manager(singleQueue);

		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			avk::application_name("Auto-Vk-Toolkit Example: Framebuffers"),
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
