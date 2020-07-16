#include <exekutor.hpp>
#include <imgui.h>

class framebuffer_app : public xk::cg_element
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

public: // v== xk::cg_element overrides which will be invoked by the framework ==v
	framebuffer_app(ak::queue& aQueue)
		: mQueue{ &aQueue }
		, mSelectedAttachmentToCopy{ 0 }
		, mAdditionalTranslationY{ 0.0f }
		, mRotationSpeed{ 1.0f }
	{ }

	void initialize() override
	{
		const auto r = xk::context().main_window()->resolution();
		auto colorAttachment = xk::context().create_image_view(xk::context().create_image(r.x, r.y, vk::Format::eR8G8B8A8Unorm, 1, ak::memory_usage::device, ak::image_usage::general_color_attachment));
		auto depthAttachment = xk::context().create_image_view(xk::context().create_image(r.x, r.y, vk::Format::eD32Sfloat, 1, ak::memory_usage::device, ak::image_usage::general_depth_stencil_attachment));
		auto colorAttachmentDescription = ak::attachment::declare_for(colorAttachment, ak::on_load::clear, ak::color(0),		ak::on_store::store);
		auto depthAttachmentDescription = ak::attachment::declare_for(depthAttachment, ak::on_load::clear, ak::depth_stencil(),	ak::on_store::store);
		mOneFramebuffer = xk::context().create_framebuffer(
			{ colorAttachmentDescription, depthAttachmentDescription },		// Attachment declarations can just be copied => use initializer_list.
			ak::make_vector( // Transfer ownership of both attachments into the vector and into create_framebuffer:
				std::move(colorAttachment), 
				std::move(depthAttachment)
			)
		);
		
		// Create graphics pipeline for rasterization with the required configuration:
		mPipeline = xk::context().create_graphics_pipeline_for(
			ak::vertex_input_location(0, &Vertex::pos),								// Describe the position vertex attribute
			ak::vertex_input_location(1, &Vertex::color),							// Describe the color vertex attribute
			ak::vertex_shader("shaders/passthrough.vert"),							// Add a vertex shader
			ak::fragment_shader("shaders/color.frag"),								// Add a fragment shader
			ak::cfg::front_face::define_front_faces_to_be_clockwise(),				// Front faces are in clockwise order
			ak::cfg::viewport_depth_scissors_config::from_framebuffer(xk::context().main_window()->backbuffer_at_index(0)),	// Align viewport with main window's resolution
			colorAttachmentDescription,
			depthAttachmentDescription
		);

		// Create vertex buffers --- namely one for each frame in flight.
		// We create multiple vertex buffers because we'll update the data every frame and frames run concurrently.
		// However, do not upload vertices yet. we'll do that in the render() method.
		auto numFramesInFlight = xk::context().main_window()->number_of_frames_in_flight();
		for (int i = 0; i < numFramesInFlight; ++i) {
			mVertexBuffers.emplace_back(
				xk::context().create_buffer(
					ak::memory_usage::device, {},								// Create the buffer on the device, i.e. in GPU memory, (no additional usage flags).
					ak::vertex_buffer_meta::create_from_data(mVertexData)		// Infer meta data from the given buffer.
				)
			);
		}
		
		// Create index buffer. Upload data already since we won't change it ever
		// (hence the usage of ak::create_and_fill instead of just ak::create)
		mIndexBuffer = xk::context().create_buffer(
			ak::memory_usage::device, {},										// Also this buffer should "live" in GPU memory
			ak::index_buffer_meta::create_from_data(mIndices)					// Pass/create meta data about the indices
		);
		// Fill it with data already here, in initialize(), because this buffer will stay constant forever:
		mIndexBuffer->fill(			
			mIndices.data(), 0,													// Since we also want to upload the data => pass a data pointer
			ak::sync::wait_idle()												// We HAVE TO synchronize this command. The easiest way is to just wait for idle.
		);

		// Get hold of the "ImGui Manager" and add a callback that draws UI elements:
		auto imguiManager = xk::current_composition()->element_by_type<xk::imgui_manager>();
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
		// Modify our vertex data according to our rotation animation and upload this frame's vertex data:
		auto rotAngle = glm::radians(90.0f) * xk::time().time_since_start() * mRotationSpeed;
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
		auto mainWnd = xk::context().main_window();
		auto inFlightIndex = mainWnd->in_flight_index_for_frame();

		// ... update its vertex data:
		mVertexBuffers[inFlightIndex]->fill(
			vertexDataCurrentFrame.data(), 0,
			// Sync this fill-operation with pipeline memory barriers:
			ak::sync::with_barriers(xk::context().main_window()->command_buffer_lifetime_handler())
			// ^ This handler is a convenience method which hands over the (internally created, but externally
			//   lifetime-handled) command buffer to the main window's swap chain. It will be deleted when it
			//   is no longer needed (which is in current-frame + frames-in-flight-frames time).
			//   ak::sync::with_barriers() offers more fine-grained control over barrier-based synchronization.
		);

		// Get a command pool to allocate command buffers from:
		auto& commandPool = xk::context().get_command_pool_for_single_use_command_buffers(*mQueue);

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
			auto transferCmdBuffer = ak::copy_image_to_another(mOneFramebuffer->image_view_at(mSelectedAttachmentToCopy)->get_image(), mainWnd->current_backbuffer()->image_view_at(0)->get_image(), ak::sync::with_barriers_by_return());
			// TODO: Add barriers to the command buffer to sync before and after
			mQueue->submit(*transferCmdBuffer);
			mainWnd->handle_lifetime(std::move(transferCmdBuffer));
		}
		else {
			// Blit:
			auto transferCmdBuffer = ak::blit_image(mOneFramebuffer->image_view_at(mSelectedAttachmentToCopy)->get_image(), mainWnd->current_backbuffer()->image_view_at(0)->get_image(), ak::sync::with_barriers_by_return());
			// TODO: Add barriers to the command buffer to sync before and after
			mQueue->submit(*transferCmdBuffer);
			mainWnd->handle_lifetime(std::move(transferCmdBuffer));
		}
	}


private: // v== Member variables ==v

	ak::queue* mQueue;
	int mSelectedAttachmentToCopy;
	int mUseCopyOrBlit;
	ak::framebuffer mOneFramebuffer;
	std::vector<ak::buffer> mVertexBuffers;
	ak::buffer mIndexBuffer;
	ak::graphics_pipeline mPipeline;
	float mAdditionalTranslationY;
	float mRotationSpeed;

}; // vertex_buffers_app

int main() // <== Starting point ==
{
	try {
		// Create a window and open it
		auto mainWnd = xk::context().create_window("Framebuffers");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->set_presentaton_mode(xk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = xk::context().create_queue({}, ak::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->add_queue_family_ownership(singleQueue);
		mainWnd->set_present_queue(singleQueue);
		
		// Create an instance of our main ak::element which contains all the functionality:
		auto app = framebuffer_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = xk::imgui_manager(singleQueue);

		// GO:
		xk::execute(
			xk::application_name("Exekutor + Auto-Vk Example: Framebuffers"),
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
