#include <cg_base.hpp>
#include <imgui.h>

class vertex_buffers_app : public cgb::cg_element
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

public: // v== cgb::cg_element overrides which will be invoked by the framework ==v
	vertex_buffers_app()
		: mAdditionalTranslationY{ 0.0f }
		, mRotationSpeed{ 1.0f }
	{ }

	void initialize() override
	{
		// Create vertex buffers, but don't upload vertices yet; we'll do that in the render() method.
		// Create multiple vertex buffers because we'll update the data every frame and frames run concurrently.
		cgb::invoke_for_all_in_flight_frames(cgb::context().main_window(), [this](int64_t inFlightIndex){
			mVertexBuffers.push_back(
				cgb::create(
					cgb::vertex_buffer_meta::create_from_data(mVertexData),	// Pass/create meta data about the vertex data
					cgb::memory_usage::device								// We want our buffer to "live" in GPU memory
				)
			);
		});

		// Create index buffer. Upload data already since we won't change it ever
		// (hence the usage of cgb::create_and_fill instead of just cgb::create)
		mIndexBuffer = cgb::create_and_fill(
			cgb::index_buffer_meta::create_from_data(mIndices),	// Pass/create meta data about the indices
			cgb::memory_usage::device,							// Also this buffer should "live" in GPU memory
			mIndices.data(),									// Since we also want to upload the data => pass a data pointer
			cgb::sync::wait_idle()								// We HAVE TO synchronize this command. The easiest way is to just wait for idle.
		);

		using namespace cgb::att;
		
		// Create graphics pipeline for rasterization with the required configuration:
		mPipeline = cgb::graphics_pipeline_for(
			cgb::vertex_input_location(0, &Vertex::pos),								// Describe the position vertex attribute
			cgb::vertex_input_location(1, &Vertex::color),								// Describe the color vertex attribute
			cgb::vertex_shader("shaders/passthrough.vert"),								// Add a vertex shader
			cgb::fragment_shader("shaders/color.frag"),									// Add a fragment shader
			cgb::cfg::front_face::define_front_faces_to_be_clockwise(),					// Front faces are in clockwise order
			cgb::cfg::viewport_depth_scissors_config::from_window(),					// Align viewport with main window's resolution
			cgb::attachment::declare(cgb::image_format::from_window_color_buffer(), on_load::clear, color(0), on_store::store) // But not in presentable format, because ImGui comes after
		);

		// Create and record command buffers for drawing the pyramid. Create one for each in-flight-frame.
		mCmdBfrs = record_command_buffers_for_all_in_flight_frames(cgb::context().main_window(), cgb::context().graphics_queue(), [&](cgb::command_buffer_t& commandBuffer, int64_t inFlightIndex) {
			commandBuffer.begin_render_pass_for_framebuffer(mPipeline->get_renderpass(), cgb::context().main_window()->backbufer_for_frame(inFlightIndex)); // TODO: only works because #concurrent == #present
			commandBuffer.handle().bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline->handle());
			commandBuffer.draw_indexed(*mIndexBuffer, *mVertexBuffers[inFlightIndex]);
			commandBuffer.end_render_pass();
		});

		auto imguiManager = cgb::current_composition().element_by_type<cgb::imgui_manager>();
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
		// On C pressed,
		if (cgb::input().key_pressed(cgb::key_code::c)) {
			// center the cursor:
			auto resolution = cgb::context().main_window()->resolution();
			cgb::context().main_window()->set_cursor_pos({ resolution[0] / 2.0, resolution[1] / 2.0 });
		}

		// On Esc pressed,
		if (cgb::input().key_pressed(cgb::key_code::escape)) {
			// stop the current composition:
			cgb::current_composition().stop();
		}
	}

	void render() override
	{
		// Modify our vertex data according to our rotation animation and upload this frame's vertex data:
		auto rotAngle = glm::radians(90.0f) * cgb::time().time_since_start() * mRotationSpeed;
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
		auto mainWnd = cgb::context().main_window();
		auto inFlightIndex = mainWnd->in_flight_index_for_frame();

		// ... update its vertex data:
		cgb::fill(
			mVertexBuffers[inFlightIndex],
			vertexDataCurrentFrame.data(),
			// Sync this fill-operation with pipeline memory barriers:
			cgb::sync::with_barriers(cgb::context().main_window()->command_buffer_lifetime_handler())
			// ^ This handler is a convenience method which hands over the (internally created, but externally
			//   lifetime-handled) command buffer to the main window's swap chain. It will be deleted when it
			//   is no longer needed (which is in current-frame + frames-in-flight-frames time).
			//   cgb::sync::with_barriers() offers more fine-grained control over barrier-based synchronization.
		);

		// Finally, submit the right command buffer in order to issue the draw call:
		mainWnd->submit_for_backbuffer_ref(mCmdBfrs[inFlightIndex]);
	}


private: // v== Member variables ==v
	
	std::vector<cgb::vertex_buffer> mVertexBuffers;
	cgb::index_buffer mIndexBuffer;
	cgb::graphics_pipeline mPipeline;
	std::vector<cgb::command_buffer> mCmdBfrs;
	float mAdditionalTranslationY;
	float mRotationSpeed;

}; // vertex_buffers_app

int main() // <== Starting point ==
{
	try {
		cgb::settings::gApplicationName = "cg_base Example: Vertex Buffers";
		cgb::settings::gQueueSelectionPreference = cgb::device_queue_selection_strategy::prefer_everything_on_single_queue;

		// Create a window and open it
		auto mainWnd = cgb::context().create_window("Vertex Buffers main window");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->set_presentaton_mode(cgb::presentation_mode::immediate);
		mainWnd->open(); 

		// Create an instance of our main cgb::element which contains all the functionality:
		auto app = vertex_buffers_app();
		// Create another element for drawing the UI with ImGui
		auto ui = cgb::imgui_manager();

		// Tie everything together and let's roll:
		auto vertexBuffers = cgb::setup(app, ui);
		vertexBuffers.start();
	}
	catch (cgb::logic_error&) {}
	catch (cgb::runtime_error&) {}
}
