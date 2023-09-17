
#include "imgui.h"

#include "configure_and_compose.hpp"
#include "imgui_manager.hpp"
#include "invokee.hpp"
#include "material_image_helpers.hpp"
#include "model.hpp"
#include "serializer.hpp"
#include "sequential_invoker.hpp"
#include "quake_camera.hpp"
#include "math_utils.hpp"

// There are several options for this example application:
enum struct options
{
	one_dds_file,
	one_ktx_file,
	six_jpeg_files
};

// Set the following define one of the options above to 
// load different cube maps:
const auto gSelectedOption = options::one_dds_file;

class texture_cubemap_app : public avk::invokee
{
	struct data_for_draw_call
	{
		avk::buffer mPositionsBuffer;
		avk::buffer mNormalsBuffer;
		avk::buffer mIndexBuffer;
	};

	struct view_projection_matrices {
		glm::mat4 mProjectionMatrix;
		glm::mat4 mModelViewMatrix;
		glm::mat4 mInverseModelViewMatrix;
		float mLodBias = 0.0f;
	};

public: // v== avk::invokee overrides which will be invoked by the framework ==v
	texture_cubemap_app(avk::queue& aQueue)
		: mQueue{ &aQueue }
		, mScale{1.0f, 1.0f, 1.0f}
	{}

	void initialize() override
	{
		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = avk::context().create_descriptor_cache();

		// Load cube map from file or from cache file:
		const std::string cacheFilePath("assets/cubemap.cache");
		auto serializer = avk::serializer(cacheFilePath);

		// Load a cubemap image file
		// The cubemap texture coordinates start in the upper right corner of the skybox faces,
		// which coincides with the memory layout of the textures. Therefore we don't need to flip them along the y axis.
		// Note that lookup operations in a cubemap are defined in a left-handed coordinate system,
		// i.e. when looking at the positive Z face from inside the cube, the positive X face is to the right.
		avk::image cubemapImage;
		avk::command::action_type_command loadImageCommand;

		// Load the textures for all cubemap faces from one file (.ktx or .dds format), or from six individual files
		if constexpr (gSelectedOption == options::one_dds_file) {
			std::tie(cubemapImage, loadImageCommand) = avk::create_cubemap_from_file_cached(
				serializer, 
				"assets/yokohama_at_night-All-Mipmaps-Srgb-RGBA8-DXT1-SRGB.dds", 
				true, // <-- load in HDR if possible 
				true, // <-- load in sRGB if applicable
				false // <-- flip along the y-axis
			);
		}
		else if constexpr (gSelectedOption == options::one_ktx_file) {
			std::tie(cubemapImage, loadImageCommand) = avk::create_cubemap_from_file_cached(
				serializer,
				"assets/yokohama_at_night-All-Mipmaps-Srgb-RGB8-DXT1-SRGB.ktx",
				true, // <-- load in HDR if possible 
				true, // <-- load in sRGB if applicable
				false // <-- flip along the y-axis
			);
		}
		else if constexpr (gSelectedOption == options::six_jpeg_files) {
			std::tie(cubemapImage, loadImageCommand) = avk::create_cubemap_from_file_cached(
				serializer,
				{ "assets/posx.jpg", "assets/negx.jpg", "assets/posy.jpg", "assets/negy.jpg", "assets/posz.jpg", "assets/negz.jpg" },
				true, // <-- load in HDR if possible 
				true, // <-- load in sRGB if applicable
				false // <-- flip along the y-axis
			);
		}
		else {
			throw std::logic_error("invalid option selected");
		}
		avk::context().record_and_submit_with_fence({ std::move(loadImageCommand) }, *mQueue)->wait_until_signalled();

		auto cubemapSampler = avk::context().create_sampler(avk::filter_mode::trilinear, avk::border_handling_mode::clamp_to_edge, static_cast<float>(cubemapImage->create_info().mipLevels));
		auto cubemapImageView = avk::context().create_image_view(cubemapImage, {}, avk::image_usage::general_cube_map_texture);
		mImageSamplerCubemap = avk::context().create_image_sampler(cubemapImageView, cubemapSampler);
	
		// Load a cube as the skybox from file
		// Since the cubemap uses a left-handed coordinate system, we declare the cube to be defined in the same coordinate system as well.
		// This simplifies coordinate transformations later on. To transform the cube vertices back to right-handed world coordinates for display,
		// we adjust its model matrix accordingly. Note that this also changes the winding order of faces, i.e. front faces
		// of the cube that have CCW order when viewed from the outside now have CCW order when viewed from inside the cube.
		{
			auto cube = avk::model_t::load_from_file("assets/cube.obj", aiProcess_Triangulate | aiProcess_PreTransformVertices);

			auto& newElement = mDrawCallsSkybox.emplace_back();

			// 2. Build all the buffers for the GPU
			std::vector<avk::mesh_index_t> indices = { 0 };

			auto modelMeshSelection = avk::make_model_references_and_mesh_indices_selection(cube, indices);

			auto [mPositionsBuffer, mIndexBuffer, geometryCommands] = avk::create_vertex_and_index_buffers({ modelMeshSelection });
			avk::context().record_and_submit_with_fence({ std::move(geometryCommands) }, *mQueue)->wait_until_signalled();

			newElement.mPositionsBuffer = std::move(mPositionsBuffer);
			newElement.mIndexBuffer = std::move(mIndexBuffer);
		}

		// Load object from file
		{
			auto object = avk::model_t::load_from_file("assets/stanford_bunny.obj", aiProcess_Triangulate | aiProcess_PreTransformVertices);

			auto& newElement = mDrawCallsReflect.emplace_back();

			// 2. Build all the buffers for the GPU
			std::vector<avk::mesh_index_t> indices = { 0 };

			auto modelMeshSelection = avk::make_model_references_and_mesh_indices_selection(object, indices);

			auto [posBuffer, indBuffer, posIndCommand] = avk::create_vertex_and_index_buffers({ modelMeshSelection });
			auto [nrmBuffer, nrmCommand] = avk::create_normals_buffer<avk::vertex_buffer_meta>({ modelMeshSelection });

			newElement.mPositionsBuffer = std::move(posBuffer);
			newElement.mIndexBuffer = std::move(indBuffer);
			newElement.mNormalsBuffer = std::move(nrmBuffer);

			avk::context().record_and_submit_with_fence({ 
				std::move(posIndCommand),
				std::move(nrmCommand)
			}, *mQueue)->wait_until_signalled();
		}

		mViewProjBufferSkybox = avk::context().create_buffer(
			avk::memory_usage::host_coherent, {},
			avk::uniform_buffer_meta::create_from_data(view_projection_matrices())
		);

		mViewProjBufferReflect = avk::context().create_buffer(
			avk::memory_usage::host_coherent, {},
			avk::uniform_buffer_meta::create_from_data(view_projection_matrices())
		);

		mPipelineSkybox = avk::context().create_graphics_pipeline_for(
			// Specify which shaders the pipeline consists of:
			avk::vertex_shader("shaders/skybox.vert"),
			avk::fragment_shader("shaders/skybox.frag"),
			// The next line defines the format and location of the vertex shader inputs:
			// (The dummy values (like glm::vec3) tell the pipeline the format of the respective input)
			avk::from_buffer_binding(0)->stream_per_vertex<glm::vec3>()->to_location(0), // <-- corresponds to vertex shader's inPosition
			// Some further settings:
			avk::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)),
			// We'll render to the back buffer, which has a color attachment always, and in our case additionally a depth 
			// attachment, which has been configured when creating the window. See main() function!
			avk::context().main_window()->get_renderpass(), // Just use the same renderpass
			// The following define additional data which we'll pass to the pipeline:
			avk::descriptor_binding(0, 0, mViewProjBufferSkybox),
			avk::descriptor_binding(0, 1, mImageSamplerCubemap->as_combined_image_sampler(avk::layout::general))
		);

		mPipelineReflect = avk::context().create_graphics_pipeline_for(
			// Specify which shaders the pipeline consists of:
			avk::vertex_shader("shaders/reflect.vert"),
			avk::fragment_shader("shaders/reflect.frag"),
			// The next 2 lines define the format and location of the vertex shader inputs:
			// (The dummy values (like glm::vec3) tell the pipeline the format of the respective input)
			avk::from_buffer_binding(0)->stream_per_vertex<glm::vec3>()->to_location(0), // <-- corresponds to vertex shader's inPosition
			avk::from_buffer_binding(1)->stream_per_vertex<glm::vec3>()->to_location(1), // <-- corresponds to vertex shader's inNormal
			// Some further settings:
			avk::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)),
			// We'll render to the back buffer, which has a color attachment always, and in our case additionally a depth 
			// attachment, which has been configured when creating the window. See main() function!
			avk::context().main_window()->get_renderpass(), // Just use the same renderpass
			// The following define additional data which we'll pass to the pipeline:
			avk::descriptor_binding(0, 0, mViewProjBufferReflect),
			avk::descriptor_binding(0, 1, mImageSamplerCubemap->as_combined_image_sampler(avk::layout::general))
		);

		mUpdater.emplace();

		// Update the pipelines if one of their shader files has changed (enable hot reloading):
		mUpdater->on(avk::shader_files_changed_event(mPipelineSkybox.as_reference())).update(mPipelineSkybox);
		mUpdater->on(avk::shader_files_changed_event(mPipelineReflect.as_reference())).update(mPipelineReflect);

		// Add the camera to the composition (and let it handle the updates)
		mQuakeCam.set_translation({ 0.0f, 0.0f, 2.5f });
		mQuakeCam.set_perspective_projection(glm::radians(60.0f), avk::context().main_window()->aspect_ratio(), 0.3f, 1000.0f);
		//mQuakeCam.set_orthographic_projection(-5, 5, -5, 5, 0.5, 100);
		avk::current_composition()->add_element(mQuakeCam);

		mUpdater->on(avk::swapchain_resized_event(avk::context().main_window()))
			.update(mPipelineSkybox, mPipelineReflect)
			.invoke([this]() {
				this->mQuakeCam.set_aspect_ratio(avk::context().main_window()->aspect_ratio());
		});

		auto imguiManager = avk::current_composition()->element_by_type<avk::imgui_manager>();
		if(nullptr != imguiManager) {
			imguiManager->add_callback([this](){
		        ImGui::Begin("Info & Settings");
				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
				ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), "[F1]: Toggle input-mode");
				ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), " (UI vs. scene navigation)");
				ImGui::End();
			});
		}
	}

	void update_uniform_buffers()
	{
		// mirror x axis to transform cubemap coordinates to righ-handed coordinates
		auto mirroredViewMatrix = avk::mirror_matrix(mQuakeCam.view_matrix(), avk::principal_axis::x);

		view_projection_matrices viewProjMat{
			mQuakeCam.projection_matrix(),
			mQuakeCam.view_matrix(),
			glm::inverse(mirroredViewMatrix),
			0.0f
		};

		auto emptyCmd = mViewProjBufferReflect->fill(&viewProjMat, 0);

		// scale skybox, mirror x axis, cancel out translation
		viewProjMat.mModelViewMatrix = avk::cancel_translation_from_matrix(mirroredViewMatrix * mModelMatrixSkybox);

		auto emptyToo = mViewProjBufferSkybox->fill(&viewProjMat, 0);
	}

	void render() override
	{
		update_uniform_buffers();
		
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
				// Begin and end one renderpass:
				avk::command::render_pass(mPipelineSkybox->renderpass_reference(), avk::context().main_window()->current_backbuffer_reference(), avk::command::gather(
					// First, draw the skybox:
					avk::command::bind_pipeline(mPipelineSkybox.as_reference()),
					avk::command::bind_descriptors(mPipelineSkybox->layout(), mDescriptorCache->get_or_create_descriptor_sets({
						avk::descriptor_binding(0, 0, mViewProjBufferSkybox),
						avk::descriptor_binding(0, 1, mImageSamplerCubemap->as_combined_image_sampler(avk::layout::general))
					})),
					avk::command::one_for_each(mDrawCallsSkybox, [](const data_for_draw_call& drawCall) {
						return avk::command::draw_indexed(drawCall.mIndexBuffer.as_reference(), drawCall.mPositionsBuffer.as_reference());
					}),
					
					// Then, draw an object which reflects it:
					avk::command::bind_pipeline(mPipelineReflect.as_reference()),
					avk::command::bind_descriptors(mPipelineReflect->layout(), mDescriptorCache->get_or_create_descriptor_sets({
						avk::descriptor_binding(0, 0, mViewProjBufferReflect),
						avk::descriptor_binding(0, 1, mImageSamplerCubemap->as_combined_image_sampler(avk::layout::general))
					})),
					avk::command::one_for_each(mDrawCallsReflect, [](const data_for_draw_call& drawCall) {
						return avk::command::draw_indexed(drawCall.mIndexBuffer.as_reference(), drawCall.mPositionsBuffer.as_reference(), drawCall.mNormalsBuffer.as_reference());
					})
				))
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

	void update() override
	{
		if (avk::input().key_pressed(avk::key_code::escape) || avk::context().main_window()->should_be_closed()) {
			// Stop the current composition:
			avk::current_composition()->stop();
		}
		if (avk::input().key_pressed(avk::key_code::f1)) {
			auto *imguiManager = avk::current_composition()->element_by_type<avk::imgui_manager>();
			if (mQuakeCam.is_enabled()) {
				mQuakeCam.disable();
				if (nullptr != imguiManager) { imguiManager->enable_user_interaction(true); }
			}
			else {
				mQuakeCam.enable();
				if (nullptr != imguiManager) { imguiManager->enable_user_interaction(false); }
			}
		}
	}

private: // v== Member variables ==v

	avk::queue* mQueue;
	avk::descriptor_cache mDescriptorCache;

	avk::buffer mViewProjBufferSkybox;
	avk::buffer mViewProjBufferReflect;

	avk::image_sampler mImageSamplerCubemap;

	std::vector<data_for_draw_call> mDrawCallsSkybox;
	avk::graphics_pipeline mPipelineSkybox;

	std::vector<data_for_draw_call> mDrawCallsReflect;
	avk::graphics_pipeline mPipelineReflect;

	avk::quake_camera mQuakeCam;

	glm::vec3 mScale;

	const float mScaleSkybox = 100.f;
	const glm::mat4 mModelMatrixSkybox = glm::scale(glm::vec3(mScaleSkybox));
}; 
// texture_cubemap_app

int main() // <== Starting point ==
{
	int result = EXIT_FAILURE;
	try {
		// Create a window and open it
		auto* mainWnd = avk::context().create_window("Texture Cubemap");
		mainWnd->set_resolution({ 800, 600 });
		mainWnd->request_srgb_framebuffer(true);
		mainWnd->enable_resizing(true);
		mainWnd->set_additional_back_buffer_attachments({ 
			avk::attachment::declare(vk::Format::eD32Sfloat, avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::depth_stencil, avk::on_store::dont_care)
		});
		mainWnd->set_presentaton_mode(avk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = avk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->set_queue_family_ownership(singleQueue.family_index());
		mainWnd->set_present_queue(singleQueue);
		
		// Create an instance of our main avk::element which contains all the functionality:
		auto app = texture_cubemap_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = avk::imgui_manager(singleQueue);

		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			avk::application_name("Auto-Vk-Toolkit Example: Texture Cubemap"),
			mainWnd,
			app,
			ui
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
