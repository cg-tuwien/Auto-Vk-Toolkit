
#include "imgui.h"

#include "configure_and_compose.hpp"
#include "imgui_manager.hpp"
#include "invokee.hpp"
#include "sequential_invoker.hpp"

class present_from_compute_app : public avk::invokee
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
	present_from_compute_app(avk::queue& aTransferQueue, avk::queue& aGraphicsQueue, avk::queue& aComputeQueue)
		: mTransferQueue{ aTransferQueue }
		, mGraphicsQueue{ aGraphicsQueue }
		, mComputeQueue { aComputeQueue  }
	{ }

	void initialize() override
	{
		// Create graphics pipeline:
		mGraphicsPipeline = avk::context().create_graphics_pipeline_for(
			avk::from_buffer_binding(0) -> stream_per_vertex(&Vertex::pos)   -> to_location(0),	// Describe the position vertex attribute
			avk::from_buffer_binding(0) -> stream_per_vertex(&Vertex::color) -> to_location(1), // Describe the color vertex attribute
			avk::vertex_shader("shaders/passthrough.vert"),										// Add a vertex shader
			avk::fragment_shader("shaders/color.frag"),											// Add a fragment shader
			avk::cfg::front_face::define_front_faces_to_be_clockwise(),							// Front faces are in clockwise order
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)), // Align viewport with main window's resolution
			avk::context().main_window()->get_renderpass()
		);

		// Create compute pipeline:
		mComputePipeline = avk::context().create_compute_pipeline_for(
			"shaders/blur.comp",
			avk::descriptor_binding<avk::image_view_as_sampled_image>(0, 0, 1u),
			avk::descriptor_binding<avk::image_view_as_storage_image>(0, 1, 1u)
		);

		auto mainWnd = avk::context().main_window();

		// Create vertex buffers and framebuffers, one for each frame in flight:
		auto numFramesInFlight = avk::context().main_window()->number_of_frames_in_flight();
		for (int i = 0; i < numFramesInFlight; ++i) {
			// Note: We're only going to use the framebuffers when using and presenting from the compute queue:
			mFramebuffers.push_back( 
				avk::context().create_framebuffer(
					avk::context().create_renderpass({
							avk::attachment::declare(
								mainWnd->swap_chain_image_format(), 
								avk::on_load::clear.from_previous_layout(avk::layout::undefined), 
								avk::usage::color(0), 
								avk::on_store::store.in_layout(avk::layout::color_attachment_optimal)
							)
						},
						mainWnd->get_renderpass()->subpass_dependencies() // Use the same as the main window's renderpass
					),
					avk::context().create_image_view(avk::context().create_image(mainWnd->resolution().x, mainWnd->resolution().y, mainWnd->swap_chain_image_format(), 1, avk::memory_usage::device, avk::image_usage::general_color_attachment | avk::image_usage::sampled | avk::image_usage::shader_storage))
				)
			);

			mVertexBuffers.push_back(
				avk::context().create_buffer(avk::memory_usage::device, {},	avk::vertex_buffer_meta::create_from_data(mVertexData))
			);
		}
		
		// Create index buffer. Upload data already since we won't change it ever
		// (hence the usage of avk::create_and_fill instead of just avk::create)
		mIndexBuffer = avk::context().create_buffer(
			avk::memory_usage::device, {},										// Also this buffer should "live" in GPU memory
			avk::index_buffer_meta::create_from_data(mIndices)					// Pass/create meta data about the indices
		);

		// Fill data on the graphics queue, so that we do not have to transfer ownership from transfer -> graphics later:
		// (Doesn't matter if this is slightly slower than on the transfer queue, because we're transferring index data only once during initialization)
		auto fence = avk::context().record_and_submit_with_fence({ mIndexBuffer->fill(mIndices.data(), 0) }, mGraphicsQueue);
		fence->wait_until_signalled();

		// Get hold of the "ImGui Manager" and add a callback that draws UI elements:
		auto imguiManager = avk::current_composition()->element_by_type<avk::imgui_manager>();
		if (nullptr != imguiManager) {
			imguiManager->add_callback([this](){
		        ImGui::Begin("Info & Settings");
				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
				ImGui::Separator();
				ImGui::Text("Select queue to present from (shortcut to toggle: 'Q'):");
				ImGui::Combo("Queue", &mUiQueueSelection, "Graphics Queue\0Compute Queue\0\0");
		        ImGui::End();
			});
		}

		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = avk::context().create_descriptor_cache();
	}

	void update() override
	{
		// On Q pressed,
		if (avk::input().key_pressed(avk::key_code::q) || mUiQueueSelection != mUiQueueSelectionPrevFrame) {
			// Switch the queue:
			if (avk::input().key_pressed(avk::key_code::q)) {
				mQueueToPresentFrom = mQueueToPresentFrom == presentation_queue::compute ? presentation_queue::graphics : presentation_queue::compute;
			}
			else if (mUiQueueSelection != mUiQueueSelectionPrevFrame) {
				mQueueToPresentFrom = 0 == mUiQueueSelection ? presentation_queue::graphics : presentation_queue::compute;
			}

			if (presentation_queue::compute == mQueueToPresentFrom) {
				avk::context().main_window()->set_image_usage_properties(
					avk::context().main_window()->get_config_image_usage_properties() | avk::image_usage::shader_storage
				);
				avk::context().main_window()->set_queue_family_ownership(mComputeQueue.family_index());
				avk::context().main_window()->set_present_queue(mComputeQueue);

				if (!mFromComputeFirstFrameId.has_value()) {
					mFromComputeFirstFrameId = avk::context().main_window()->current_frame();
				}

				mUiQueueSelection = 1; // from compute
			}
			else { // present from graphics:
				avk::context().main_window()->set_image_usage_properties(
					avk::exclude(avk::context().main_window()->get_config_image_usage_properties(), avk::image_usage::shader_storage)
				);
				avk::context().main_window()->set_queue_family_ownership(mGraphicsQueue.family_index());
				avk::context().main_window()->set_present_queue(mGraphicsQueue);

				mUiQueueSelection = 0; // from graphics
			}
		}
		mUiQueueSelectionPrevFrame = mUiQueueSelection;

		// On Esc pressed,
		if (avk::input().key_pressed(avk::key_code::escape) || avk::context().main_window()->should_be_closed()) {
			// stop the current composition:
			avk::current_composition()->stop();
		}
	}

	// Perform the operations on the transfer queue:
	avk::semaphore transfer()
	{
		using namespace avk;

		const auto* mainWnd = avk::context().main_window();
		const auto inFlightIndex = mainWnd->in_flight_index_for_frame();

		// Modify our vertex data according to our rotation animation and upload this frame's vertex data:
		auto rotAngle = glm::radians(90.0f) * avk::time().time_since_start();
		auto rotMatrix = glm::rotate(rotAngle, glm::vec3(0.f, 1.f, 0.f));
		auto translateZ = glm::translate(glm::vec3{ 0.0f, 0.0f, -0.5f });
		auto invTranslZ = glm::inverse(translateZ);

		// Store modified vertices in vertexDataCurrentFrame:
		std::vector<Vertex> vertexDataCurrentFrame;
		for (const Vertex& vtx : mVertexData) {
			vertexDataCurrentFrame.push_back({ glm::vec3(invTranslZ * rotMatrix * translateZ * glm::vec4{ vtx.pos, 1.0f }), vtx.color });
		}

		// Transfer vertex data on the (fast) transfer queue:
		auto vertexBufferFillSemaphore = avk::context().record_and_submit_with_semaphore({ // Gotta use a semaphore to synchronize different queues
				command::conditional(
					[frameId = avk::context().main_window()->current_frame()]() { // Skip ownership transfer for the very first frame, because in the very first frame, there is no owner yet
						return frameId >= 3; // Because we have three concurrent frames
					},
					[&]() {
						// Acquire exclusive ownership from the graphics queue:
						return sync::buffer_memory_barrier(mVertexBuffers[inFlightIndex].as_reference(), stage::none + access::none >> stage::auto_stage + access::auto_access)
							.with_queue_family_ownership_transfer(mGraphicsQueue.family_index(), mTransferQueue.family_index());
					}
				),

				// Perform the copy:
				mVertexBuffers[inFlightIndex]->fill(vertexDataCurrentFrame.data(), 0),

				// Release exclusive ownership from the transfer queue:
				sync::buffer_memory_barrier(mVertexBuffers[inFlightIndex].as_reference(), stage::auto_stage + access::auto_access >> stage::none + access::none)
					.with_queue_family_ownership_transfer(mTransferQueue.family_index(), mGraphicsQueue.family_index())
			}, mTransferQueue,
			stage::auto_stage // Let the framework determine the (source) stage after which the semaphore can be signaled (will be stage::copy due to buffer_t::fill)
		);

		return vertexBufferFillSemaphore;
	}

	// Perform the operations on the graphics queue:
	std::optional<avk::semaphore> graphics(avk::semaphore aVertexBufferFillSemaphore, const avk::framebuffer_t& aFramebuffer, std::optional<avk::semaphore> aImageAvailableSemaphore)
	{
		using namespace avk;

		const auto* mainWnd = avk::context().main_window();
		const auto inFlightIndex = mainWnd->in_flight_index_for_frame();
		std::optional<avk::semaphore> graphicsFinishedSemaphore;

		auto& commandPool = avk::context().get_command_pool_for_single_use_command_buffers(mGraphicsQueue);

		// Create a command buffer and render into the current swap chain image:
		auto cmdBfrForGraphics = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		// Get handle of the ImGui Manager invokee:
		auto imguiManager = avk::current_composition()->element_by_type<avk::imgui_manager>();

		// Submit the draw calls to the graphics queue:
		auto submissionData = avk::context().record({
				command::conditional(
					[frameId = avk::context().main_window()->current_frame(), this]() { // Skip ownership transfer for the very first frame, because in the very first frame, there is no owner yet
						return presentation_queue::compute == mQueueToPresentFrom && frameId >= mFromComputeFirstFrameId.value_or(0) + 3; // Because we have three concurrent frames
					},
					[&]() {
						// Acquire exclusive ownership from the compute queue:
						return sync::image_memory_barrier(mFramebuffers[inFlightIndex]->image_at(0), stage::none + access::none >> stage::auto_stage + access::auto_access)
							.with_queue_family_ownership_transfer(mComputeQueue.family_index(), mGraphicsQueue.family_index());
					}
				),

				// Acquire exclusive ownership for the graphics queue:
				sync::buffer_memory_barrier(mVertexBuffers[inFlightIndex].as_reference(), stage::none + access::none >> stage::vertex_attribute_input + access::vertex_attribute_read)
					.with_queue_family_ownership_transfer(mTransferQueue.family_index(), mGraphicsQueue.family_index()),

				// Begin and end one renderpass:
				command::render_pass(aFramebuffer.renderpass_reference(), aFramebuffer, {
					// And within, bind a pipeline and draw three vertices:
					command::bind_pipeline(mGraphicsPipeline.as_reference()),
					command::draw_indexed(mIndexBuffer.as_reference(), mVertexBuffers[inFlightIndex].as_reference())
				}),
			
				// We've got to synchronize between the two draw calls. Scene drawing must have completed before ImGui
				// may proceed to render something into the same color buffer:
				sync::global_memory_barrier(stage::auto_stage + access::auto_access >> stage::auto_stage + access::auto_access),

				// Let ImGui render now:
				imguiManager->render_command(aFramebuffer),

				// Release exclusive ownership from the the graphics queue
				sync::buffer_memory_barrier(mVertexBuffers[inFlightIndex].as_reference(), stage::vertex_attribute_input + access::vertex_attribute_read >> stage::none + access::none)
					.with_queue_family_ownership_transfer(mGraphicsQueue.family_index(), mTransferQueue.family_index()),

				command::conditional(
					[this]() {
						return presentation_queue::compute == mQueueToPresentFrom;
					},
					[&]() {
						// Release exclusive ownership from the graphics queue:
						return sync::image_memory_barrier(mFramebuffers[inFlightIndex]->image_at(0), stage::auto_stage + access::auto_access >> stage::none + access::none)
							.with_queue_family_ownership_transfer(mGraphicsQueue.family_index(), mComputeQueue.family_index());
					}
				)
			})
			.into_command_buffer(cmdBfrForGraphics)
			.then_submit_to(mGraphicsQueue)
			// Wait for the data transfer into the vertex buffer has completed:
			.waiting_for(aVertexBufferFillSemaphore >> stage::vertex_attribute_input)
			.store_for_now();

		if (aImageAvailableSemaphore.has_value()) {
			// Do not start to render before the image has become available:
			submissionData.waiting_for(aImageAvailableSemaphore.value() >> stage::color_attachment_output);
		}
		else {
			graphicsFinishedSemaphore = avk::context().create_semaphore();
			submissionData.signaling_upon_completion(stage::color_attachment_output >> graphicsFinishedSemaphore.value());
		}

		submissionData.submit();
		
		// Let the command buffer handle the semaphore's lifetime:
		cmdBfrForGraphics->handle_lifetime_of(std::move(aVertexBufferFillSemaphore));

		// Use a convenience function of avk::window to take care of the command buffer's lifetime:
		// It will get deleted in the future after #concurrent-frames have passed by.
		avk::context().main_window()->handle_lifetime(std::move(cmdBfrForGraphics));

		return graphicsFinishedSemaphore;
	}

	// Perform operations on the compute queue:
	void compute(avk::semaphore aImageAvailableSemaphore, avk::semaphore aGraphicsFinishedSemaphore)
	{
		using namespace avk;

		auto* mainWnd = avk::context().main_window();
		const auto inFlightIndex = mainWnd->in_flight_index_for_frame();
		const auto w = mainWnd->resolution().x;
		const auto h = mainWnd->resolution().y;

		auto& commandPool = avk::context().get_command_pool_for_single_use_command_buffers(mComputeQueue);

		// Create a command buffer and render into the current swap chain image:
		auto cmdBfrForCompute = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		// Submit the dispatch calls to the compute queue:
		avk::context().record({
				// Acquire exclusive ownership from the graphics queue:
				sync::image_memory_barrier(mFramebuffers[inFlightIndex]->image_at(0), stage::none + access::none >> stage::auto_stage + access::auto_access)
					.with_queue_family_ownership_transfer(mGraphicsQueue.family_index(), mComputeQueue.family_index()),

				sync::image_memory_barrier(mFramebuffers[inFlightIndex]->image_at(0), stage::auto_stage + access::auto_access >> stage::auto_stage + access::auto_access)
					.with_layout_transition(layout::present_src >> layout::shader_read_only_optimal),
				sync::image_memory_barrier(mainWnd->current_backbuffer_reference().image_at(0), stage::auto_stage + access::auto_access >> stage::auto_stage + access::auto_access)
					.with_layout_transition(layout::undefined >> layout::general),

				command::bind_pipeline(mComputePipeline.as_reference()),
				command::bind_descriptors(mComputePipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
						descriptor_binding(0, 0, mFramebuffers[inFlightIndex]->image_view_at(0)->as_sampled_image(layout::shader_read_only_optimal)),
						descriptor_binding(0, 1, mainWnd->current_backbuffer_reference().image_view_at(0)->as_storage_image(layout::general)),
					})),
				command::dispatch((w + 15u) / 16u, (h + 15u) / 16u, 1),

				sync::image_memory_barrier(mainWnd->current_backbuffer_reference().image_at(0), stage::auto_stage + access::auto_access >> stage::auto_stage + access::auto_access)
					.with_layout_transition(layout::general >> layout::present_src),

				// Release exclusive ownership from the compute queue:
				sync::image_memory_barrier(mFramebuffers[inFlightIndex]->image_at(0), stage::auto_stage + access::auto_access >> stage::none + access::none)
					.with_queue_family_ownership_transfer(mComputeQueue.family_index(), mGraphicsQueue.family_index())
			})
			.into_command_buffer(cmdBfrForCompute)
			.then_submit_to(mComputeQueue)
			// Do not start to render before the image has become available:
			.waiting_for(aImageAvailableSemaphore >> stage::compute_shader)
			// Do not start before graphics hasn't completed:
			.waiting_for(aGraphicsFinishedSemaphore >> stage::compute_shader)
			// Gotta use the fence to signal everything is completely finished:
			.signaling_upon_completion(mainWnd->use_current_frame_finished_fence())
			.submit();

		// Let the command buffer handle the semaphores lifetimes:
		cmdBfrForCompute->handle_lifetime_of(std::move(aImageAvailableSemaphore));
		cmdBfrForCompute->handle_lifetime_of(std::move(aGraphicsFinishedSemaphore));

		// Use a convenience function of avk::window to take care of the command buffer's lifetime:
		// It will get deleted in the future after #concurrent-frames have passed by.
		avk::context().main_window()->handle_lifetime(std::move(cmdBfrForCompute));

	}

	void render() override
	{
		auto mainWnd = avk::context().main_window();
		auto inFlightIndex = mainWnd->in_flight_index_for_frame();

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		// In this application, we have to wait for it either on the graphics queue or on the compute queue.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();

		// 1. TRANSFER:
		auto transferCompleteSemaphore = transfer();

		// 2. GRAPHICS:
		if (presentation_queue::graphics == mQueueToPresentFrom) {
			// Graphics must wait on the image to become avaiable:
			graphics(std::move(transferCompleteSemaphore), mainWnd->current_backbuffer_reference(), imageAvailableSemaphore);
		}
		else {
			// Graphics renders into the intermediate framebuffer, compute pipeline waits on the imageAvailableSemaphore (not graphics pipeline):
			auto graphicsFinishedSemaphore = graphics(std::move(transferCompleteSemaphore), mFramebuffers[inFlightIndex].as_reference(), {});
			assert(graphicsFinishedSemaphore.has_value());

			compute(std::move(imageAvailableSemaphore), std::move(graphicsFinishedSemaphore.value()));
		}
	}


private: // v== Member variables ==v
	enum struct presentation_queue { graphics, compute };

	avk::queue& mTransferQueue;
	avk::queue& mGraphicsQueue;
	avk::queue& mComputeQueue;
	int mUiQueueSelectionPrevFrame = 0;
	int mUiQueueSelection = 0;
	presentation_queue mQueueToPresentFrom = presentation_queue::graphics;
	std::optional<avk::window::frame_id_t> mFromComputeFirstFrameId; // Gotta remember the first frame this happens to get the initial queue ownership tranfers right.
	avk::graphics_pipeline mGraphicsPipeline;
	avk::compute_pipeline mComputePipeline;
	std::vector<avk::framebuffer> mFramebuffers;
	std::vector<avk::buffer> mVertexBuffers;
	avk::buffer mIndexBuffer;
	/** One descriptor cache to use for allocating all the descriptor sets from: */
	avk::descriptor_cache mDescriptorCache;
};

int main() // <== Starting point ==
{
	int result = EXIT_FAILURE;
	try {
		// Create a window and open it
		auto mainWnd = avk::context().create_window("Present from Compute");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->set_presentaton_mode(avk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->enable_resizing(false);
		mainWnd->open();

		auto& transferQueue = avk::context().create_queue(vk::QueueFlagBits::eTransfer, avk::queue_selection_preference::specialized_queue);
		auto& graphicsQueue = avk::context().create_queue(vk::QueueFlagBits::eGraphics, avk::queue_selection_preference::specialized_queue, mainWnd);
		auto& computeQueue  = avk::context().create_queue(vk::QueueFlagBits::eCompute , avk::queue_selection_preference::specialized_queue, mainWnd);
		// Initialize to graphics queue (but can be changed later):
		mainWnd->set_queue_family_ownership(graphicsQueue.family_index());
		mainWnd->set_present_queue(graphicsQueue);
		
		// Create an instance of our main "invokee" which contains all the functionality:
		auto app = present_from_compute_app(transferQueue, graphicsQueue, computeQueue);
		// ImGui always renders using the graphics queue:
		auto ui = avk::imgui_manager(graphicsQueue);

		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			avk::application_name("Auto-Vk-Toolkit Example: Present from Compute"),
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
