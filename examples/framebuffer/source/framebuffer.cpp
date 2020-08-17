#include <gvk.hpp>
#include <imgui.h>

class framebuffer_app : public gvk::invokee
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
		const auto r = gvk::context().main_window()->resolution();
		auto colorAttachment = gvk::context().create_image_view(gvk::context().create_image(r.x, r.y, vk::Format::eR8G8B8A8Unorm, 1, avk::memory_usage::device, avk::image_usage::general_color_attachment));
		auto depthAttachment = gvk::context().create_image_view(gvk::context().create_image(r.x, r.y, vk::Format::eD32Sfloat, 1, avk::memory_usage::device, avk::image_usage::general_depth_stencil_attachment));
		auto colorAttachmentDescription = avk::attachment::declare_for(colorAttachment, avk::on_load::clear, avk::color(0),		avk::on_store::store);
		auto depthAttachmentDescription = avk::attachment::declare_for(depthAttachment, avk::on_load::clear, avk::depth_stencil(),	avk::on_store::store);
		mOneFramebuffer = gvk::context().create_framebuffer(
			{ colorAttachmentDescription, depthAttachmentDescription },		// Attachment declarations can just be copied => use initializer_list.
			avk::make_vector( // Transfer ownership of both attachments into the vector and into create_framebuffer:
				std::move(colorAttachment), 
				std::move(depthAttachment)
			)
		);
		
		// Create graphics pipeline for rasterization with the required configuration:
		mPipeline = gvk::context().create_graphics_pipeline_for(
			avk::from_buffer_binding(0) -> stream_per_vertex(&Vertex::pos)   -> to_location(0),	// Describe the position vertex attribute
			avk::from_buffer_binding(0) -> stream_per_vertex(&Vertex::color) -> to_location(1), // Describe the color vertex attribute
			avk::vertex_shader("shaders/passthrough.vert"),										// Add a vertex shader
			avk::fragment_shader("shaders/color.frag"),											// Add a fragment shader
			avk::cfg::front_face::define_front_faces_to_be_clockwise(),							// Front faces are in clockwise order
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(gvk::context().main_window()->backbuffer_at_index(0)),	// Align viewport with main window's resolution
			colorAttachmentDescription,
			depthAttachmentDescription
		);

		// Create vertex buffers --- namely one for each frame in flight.
		// We create multiple vertex buffers because we'll update the data every frame and frames run concurrently.
		// However, do not upload vertices yet. we'll do that in the render() method.
		auto numFramesInFlight = gvk::context().main_window()->number_of_frames_in_flight();
		for (int i = 0; i < numFramesInFlight; ++i) {
			mVertexBuffers.emplace_back(
				gvk::context().create_buffer(
					avk::memory_usage::device, {},								// Create the buffer on the device, i.e. in GPU memory, (no additional usage flags).
					avk::vertex_buffer_meta::create_from_data(mVertexData)		// Infer meta data from the given buffer.
				)
			);
		}
		
		// Create index buffer. Upload data already since we won't change it ever
		// (hence the usage of avk::create_and_fill instead of just avk::create)
		mIndexBuffer = gvk::context().create_buffer(
			avk::memory_usage::device, {},										// Also this buffer should "live" in GPU memory
			avk::index_buffer_meta::create_from_data(mIndices)					// Pass/create meta data about the indices
		);
		// Fill it with data already here, in initialize(), because this buffer will stay constant forever:
		mIndexBuffer->fill(			
			mIndices.data(), 0,													// Since we also want to upload the data => pass a data pointer
			avk::sync::wait_idle()												// We HAVE TO synchronize this command. The easiest way is to just wait for idle.
		);

		// Get hold of the "ImGui Manager" and add a callback that draws UI elements:
		auto imguiManager = gvk::current_composition()->element_by_type<gvk::imgui_manager>();
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
		if (gvk::input().key_pressed(gvk::key_code::c)) {
			// center the cursor:
			auto resolution = gvk::context().main_window()->resolution();
			gvk::context().main_window()->set_cursor_pos({ resolution[0] / 2.0, resolution[1] / 2.0 });
		}

		// On Esc pressed,
		if (gvk::input().key_pressed(gvk::key_code::escape)) {
			// stop the current composition:
			gvk::current_composition()->stop();
		}
	}

	void render() override
	{
		// Modify our vertex data according to our rotation animation and upload this frame's vertex data:
		auto rotAngle = glm::radians(90.0f) * gvk::time().time_since_start() * mRotationSpeed;
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

		// For the right vertex buffer, ...
		auto mainWnd = gvk::context().main_window();
		auto inFlightIndex = mainWnd->in_flight_index_for_frame();

		// ... update its vertex data:
		mVertexBuffers[inFlightIndex]->fill(
			vertexDataCurrentFrame.data(), 0,
			// Sync this fill-operation with pipeline memory barriers:
			avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler())
			// ^ This handler is a convenience method which hands over the (internally created, but externally
			//   lifetime-handled) command buffer to the main window's swap chain. It will be deleted when it
			//   is no longer needed (which is in current-frame + frames-in-flight-frames time).
			//   avk::sync::with_barriers() offers more fine-grained control over barrier-based synchronization.
		);

		// Get a command pool to allocate command buffers from:
		auto& commandPool = gvk::context().get_command_pool_for_single_use_command_buffers(*mQueue);

		// Create a command buffer and render into the *current* swap chain image:
		
		auto cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		cmdBfr->begin_recording();
		cmdBfr->begin_render_pass_for_framebuffer(mPipeline->get_renderpass(), mOneFramebuffer);
		cmdBfr->handle().bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline->handle());
		cmdBfr->draw_indexed(*mIndexBuffer, *mVertexBuffers[inFlightIndex]);
		cmdBfr->end_render_pass();
		cmdBfr->end_recording();

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto& imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();
		
		// Submit the draw call and take care of the command buffer's lifetime:
		mQueue->submit(cmdBfr, imageAvailableSemaphore);
		mainWnd->handle_lifetime(std::move(cmdBfr));

		if (0 == mUseCopyOrBlit) {
			// Copy:
			auto transferCmdBuffer = avk::copy_image_to_another(mOneFramebuffer->image_view_at(mSelectedAttachmentToCopy)->get_image(), mainWnd->current_backbuffer()->image_view_at(0)->get_image(), avk::sync::with_barriers_by_return());
			// TODO: Add barriers to the command buffer to sync before and after
			mQueue->submit(*transferCmdBuffer);
			mainWnd->handle_lifetime(std::move(transferCmdBuffer));
		}
		else {
			// Blit:
			auto transferCmdBuffer = avk::blit_image(mOneFramebuffer->image_view_at(mSelectedAttachmentToCopy)->get_image(), mainWnd->current_backbuffer()->image_view_at(0)->get_image(), avk::sync::with_barriers_by_return());
			// TODO: Add barriers to the command buffer to sync before and after
			mQueue->submit(*transferCmdBuffer);
			mainWnd->handle_lifetime(std::move(transferCmdBuffer));
		}
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
	try {
		// Create a window and open it
		auto mainWnd = gvk::context().create_window("Framebuffers");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->set_presentaton_mode(gvk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = gvk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->add_queue_family_ownership(singleQueue);
		mainWnd->set_present_queue(singleQueue);
		
		// Create an instance of our main avk::element which contains all the functionality:
		auto app = framebuffer_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = gvk::imgui_manager(singleQueue);

		// GO:
		gvk::start(
			gvk::application_name("Gears-Vk + Auto-Vk Example: Framebuffers"),
			mainWnd,
			app,
			ui
		);		
	}
	catch (gvk::logic_error&) {}
	catch (gvk::runtime_error&) {}
	catch (avk::logic_error&) {}
	catch (avk::runtime_error&) {}
}
