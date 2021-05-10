#include <gvk.hpp>
#include <imgui.h>

class texture_cubemap_app : public gvk::invokee
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
		mDescriptorCache = gvk::context().create_descriptor_cache();

		// Load cube map from file or from cache file:
		const std::string cacheFilePath("assets/cubemap.cache");
		auto serializer = gvk::serializer(cacheFilePath);

		// Load a cubemap image file
		// The cubemap texture coordinates start in the upper right corner of the skybox faces,
		// which coincides with the memory layout of the textures. Therefore we don't need to flip them along the y axis.
		// Note that lookup operations in a cubemap are defined in a left-handed coordinate system,
		// i.e. when looking at the positive Z face from inside the cube, the positive X face is to the right.
		avk::image cubemapImage;

		// Load the textures for all cubemap faces from one file (.ktx or .dds format), or from six individual files
		bool loadSingleFile = true;
		if (loadSingleFile) {
			bool loadDds = false;
			std::string cubemapFile;
			if (loadDds)
			{
				cubemapFile = "assets/yokohama_at_night-All-Mipmaps-Srgb-RGBA8-DXT1-SRGB.dds";
			}
			else
			{
				cubemapFile = "assets/yokohama_at_night-All-Mipmaps-Srgb-RGB8-DXT1-SRGB.ktx";
			}
			cubemapImage = gvk::create_cubemap_from_file_cached(serializer, cubemapFile, true, true, false);
		}
		else {
			std::vector<std::string> cubemapFiles{ "assets/posx.jpg", "assets/negx.jpg", "assets/posy.jpg", "assets/negy.jpg", "assets/posz.jpg", "assets/negz.jpg" };
			cubemapImage = gvk::create_cubemap_from_file_cached(serializer, cubemapFiles, true, true, false);
		}

		// the image format is used after cubemapImage is moved, hence a copy is needed
		auto cubemapImageFormat = cubemapImage->format();

		auto cubemapSampler = gvk::context().create_sampler(avk::filter_mode::trilinear, avk::border_handling_mode::clamp_to_edge, static_cast<float>(cubemapImage->config().mipLevels));
		auto cubemapImageView = gvk::context().create_image_view(avk::owned(cubemapImage), cubemapImageFormat, avk::image_usage::general_cube_map_texture);
		mImageSamplerCubemap = gvk::context().create_image_sampler(avk::owned(cubemapImageView), avk::owned(cubemapSampler));
	
		// Load a cube as the skybox from file
		// Since the cubemap uses a left-handed coordinate system, we declare the cube to be defined in the same coordinate system as well.
		// This simplifies coordinate transformations later on. To transform the cube vertices back to right-handed world coordinates for display,
		// we adjust its model matrix accordingly. Note that this also changes the winding order of faces, i.e. front faces
		// of the cube that have CCW order when viewed from the outside now have CCW order when viewed from inside the cube.
		{
			auto cube = gvk::model_t::load_from_file("assets/cube.obj", aiProcess_Triangulate | aiProcess_PreTransformVertices);

			auto& newElement = mDrawCallsSkybox.emplace_back();

			// 2. Build all the buffers for the GPU
			std::vector<gvk::mesh_index_t> indices = { 0 };

			auto modelMeshSelection = gvk::make_models_and_meshes_selection(cube, indices);

			auto [mPositionsBuffer, mIndexBuffer] = gvk::create_vertex_and_index_buffers({ modelMeshSelection });

			newElement.mPositionsBuffer = std::move(mPositionsBuffer);
			newElement.mIndexBuffer = std::move(mIndexBuffer);
		}

		// Load object from file
		{
			auto object = gvk::model_t::load_from_file("assets/stanford_bunny.obj", aiProcess_Triangulate | aiProcess_PreTransformVertices);

			auto& newElement = mDrawCallsReflect.emplace_back();

			// 2. Build all the buffers for the GPU
			std::vector<gvk::mesh_index_t> indices = { 0 };

			auto modelMeshSelection = gvk::make_models_and_meshes_selection(object, indices);

			auto [mPositionsBuffer, mIndexBuffer] = gvk::create_vertex_and_index_buffers({ modelMeshSelection });

			newElement.mPositionsBuffer = std::move(mPositionsBuffer);
			newElement.mIndexBuffer = std::move(mIndexBuffer);

			newElement.mNormalsBuffer = gvk::create_normals_buffer<avk::vertex_buffer_meta>({ modelMeshSelection });
		}

		mViewProjBufferSkybox = gvk::context().create_buffer(
			avk::memory_usage::host_visible, {},
			avk::uniform_buffer_meta::create_from_data(view_projection_matrices())
		);

		mViewProjBufferReflect = gvk::context().create_buffer(
			avk::memory_usage::host_visible, {},
			avk::uniform_buffer_meta::create_from_data(view_projection_matrices())
		);

		mPipelineSkybox = gvk::context().create_graphics_pipeline_for(
			// Specify which shaders the pipeline consists of:
			avk::vertex_shader("shaders/skybox.vert"),
			avk::fragment_shader("shaders/skybox.frag"),
			// The next line defines the format and location of the vertex shader inputs:
			// (The dummy values (like glm::vec3) tell the pipeline the format of the respective input)
			avk::from_buffer_binding(0)->stream_per_vertex<glm::vec3>()->to_location(0), // <-- corresponds to vertex shader's inPosition
			// Some further settings:
			avk::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(gvk::context().main_window()->backbuffer_at_index(0)),
			// We'll render to the back buffer, which has a color attachment always, and in our case additionally a depth 
			// attachment, which has been configured when creating the window. See main() function!
			avk::attachment::declare(gvk::format_from_window_color_buffer(gvk::context().main_window()), avk::on_load::clear, avk::color(0), avk::on_store::store),	 // But not in presentable format, because ImGui comes after
			avk::attachment::declare(gvk::format_from_window_depth_buffer(gvk::context().main_window()), avk::on_load::clear, avk::depth_stencil(), avk::on_store::store),
			// The following define additional data which we'll pass to the pipeline:
			avk::descriptor_binding(0, 0, mViewProjBufferSkybox),
			avk::descriptor_binding(0, 1, mImageSamplerCubemap)
		);

		mPipelineReflect = gvk::context().create_graphics_pipeline_for(
			// Specify which shaders the pipeline consists of:
			avk::vertex_shader("shaders/reflect.vert"),
			avk::fragment_shader("shaders/reflect.frag"),
			// The next 2 lines define the format and location of the vertex shader inputs:
			// (The dummy values (like glm::vec3) tell the pipeline the format of the respective input)
			avk::from_buffer_binding(0)->stream_per_vertex<glm::vec3>()->to_location(0), // <-- corresponds to vertex shader's inPosition
			avk::from_buffer_binding(1)->stream_per_vertex<glm::vec3>()->to_location(1), // <-- corresponds to vertex shader's inNormal
			// Some further settings:
			avk::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(gvk::context().main_window()->backbuffer_at_index(0)),
			// We'll render to the back buffer, which has a color attachment always, and in our case additionally a depth 
			// attachment, which has been configured when creating the window. See main() function!
			avk::attachment::declare(gvk::format_from_window_color_buffer(gvk::context().main_window()), avk::on_load::load, avk::color(0), avk::on_store::store),	 // But not in presentable format, because ImGui comes after
			avk::attachment::declare(gvk::format_from_window_depth_buffer(gvk::context().main_window()), avk::on_load::load, avk::depth_stencil(), avk::on_store::dont_care),
			// The following define additional data which we'll pass to the pipeline:
			avk::descriptor_binding(0, 0, mViewProjBufferReflect),
			avk::descriptor_binding(0, 1, mImageSamplerCubemap)
		);

		mUpdater.emplace();

		// Make pipelines usable with the updater
		mPipelineSkybox.enable_shared_ownership(); 
		mPipelineReflect.enable_shared_ownership();

		mUpdater->on(gvk::swapchain_resized_event(gvk::context().main_window())).invoke([this]() {
			this->mQuakeCam.set_aspect_ratio(gvk::context().main_window()->aspect_ratio());
			});

		//first make sure render pass is updated
		mUpdater->on(gvk::swapchain_format_changed_event(gvk::context().main_window()),
			gvk::swapchain_additional_attachments_changed_event(gvk::context().main_window())
		).invoke([this]() {
			const std::vector<avk::attachment> renderpassAttachments = {
				// But not in presentable format, because ImGui comes after
				avk::attachment::declare(gvk::format_from_window_color_buffer(gvk::context().main_window()), avk::on_load::clear, avk::color(0),		avk::on_store::store),	
				avk::attachment::declare(gvk::format_from_window_depth_buffer(gvk::context().main_window()), avk::on_load::clear, avk::depth_stencil(), avk::on_store::dont_care)
			};
			auto renderPassSkybox = gvk::context().create_renderpass(renderpassAttachments);
			auto renderPassReflect = gvk::context().create_renderpass(renderpassAttachments);
			gvk::context().replace_render_pass_for_pipeline(mPipelineSkybox, std::move(renderPassSkybox));
			gvk::context().replace_render_pass_for_pipeline(mPipelineReflect, std::move(renderPassReflect));
			// ... next, at this point, we are sure that the render pass is correct -> check if there are events that would update the pipeline
			}).then_on(
				gvk::swapchain_changed_event(gvk::context().main_window()),
				gvk::shader_files_changed_event(mPipelineSkybox)
			).update(mPipelineSkybox)
			.then_on(
				gvk::swapchain_changed_event(gvk::context().main_window()),
				gvk::shader_files_changed_event(mPipelineReflect)
			).update(mPipelineReflect);;

		// Add the camera to the composition (and let it handle the updates)
		mQuakeCam.set_translation({ 0.0f, 0.0f, 2.5f });
		mQuakeCam.set_perspective_projection(glm::radians(60.0f), gvk::context().main_window()->aspect_ratio(), 0.3f, 1000.0f);
		//mQuakeCam.set_orthographic_projection(-5, 5, -5, 5, 0.5, 100);
		gvk::current_composition()->add_element(mQuakeCam);

		auto imguiManager = gvk::current_composition()->element_by_type<gvk::imgui_manager>();
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
		auto mirroredViewMatrix = gvk::mirror_matrix(mQuakeCam.view_matrix());

		view_projection_matrices viewProjMat{
			mQuakeCam.projection_matrix(),
			mQuakeCam.view_matrix(),
			glm::inverse(mirroredViewMatrix),
			0.0f
		};

		mViewProjBufferReflect->fill(&viewProjMat, 0, avk::sync::not_required());

		// scale skybox, mirror x axis, cancel out translation
		viewProjMat.mModelViewMatrix = gvk::cancel_translation_from_matrix(mirroredViewMatrix * mModelMatrixSkybox);

		mViewProjBufferSkybox->fill(&viewProjMat, 0, avk::sync::not_required());
	}

	void render() override
	{
		update_uniform_buffers();

		auto *mainWnd = gvk::context().main_window();

		auto& commandPool = gvk::context().get_command_pool_for_single_use_command_buffers(*mQueue);
		auto cmdbfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		cmdbfr->begin_recording();
		cmdbfr->begin_render_pass_for_framebuffer(mPipelineSkybox->get_renderpass(), gvk::context().main_window()->current_backbuffer());

		cmdbfr->bind_pipeline(avk::const_referenced(mPipelineSkybox));
		cmdbfr->bind_descriptors(mPipelineSkybox->layout(), mDescriptorCache.get_or_create_descriptor_sets({
			avk::descriptor_binding(0, 0, mViewProjBufferSkybox),
			avk::descriptor_binding(0, 1, mImageSamplerCubemap)
			}));

		for (auto& drawCall : mDrawCallsSkybox) {
			// Make the draw call:
			cmdbfr->draw_indexed(
				// Bind and use the index buffer:
				avk::const_referenced(drawCall.mIndexBuffer),
				// Bind the vertex input buffers in the right order (corresponding to the layout specifiers in the vertex shader)
				avk::const_referenced(drawCall.mPositionsBuffer)
			);
		}

		cmdbfr->bind_pipeline(avk::const_referenced(mPipelineReflect));
		cmdbfr->bind_descriptors(mPipelineReflect->layout(), mDescriptorCache.get_or_create_descriptor_sets({
			avk::descriptor_binding(0, 0, mViewProjBufferReflect),
			avk::descriptor_binding(0, 1, mImageSamplerCubemap)
			}));

		for (auto& drawCall : mDrawCallsReflect) {
			// Make the draw call:
			cmdbfr->draw_indexed(
				// Bind and use the index buffer:
				avk::const_referenced(drawCall.mIndexBuffer),
				// Bind the vertex input buffers in the right order (corresponding to the layout specifiers in the vertex shader)
				avk::const_referenced(drawCall.mPositionsBuffer), avk::const_referenced(drawCall.mNormalsBuffer)
			);
		}

		cmdbfr->end_render_pass();
		cmdbfr->end_recording();

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();
		
		// Submit the draw call and take care of the command buffer's lifetime:
		mQueue->submit(cmdbfr, imageAvailableSemaphore);
		mainWnd->handle_lifetime(avk::owned(cmdbfr));
	}

	void update() override
	{
		if (gvk::input().key_pressed(gvk::key_code::escape)) {
			// Stop the current composition:
			gvk::current_composition()->stop();
		}
		if (gvk::input().key_pressed(gvk::key_code::f1)) {
			auto *imguiManager = gvk::current_composition()->element_by_type<gvk::imgui_manager>();
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

	gvk::quake_camera mQuakeCam;

	glm::vec3 mScale;

	const float mScaleSkybox = 100.f;
	const glm::mat4 mModelMatrixSkybox = glm::scale(glm::vec3(mScaleSkybox));
}; 
// texture_cubemap_app

int main() // <== Starting point ==
{
	try {
		// Create a window and open it
		auto *mainWnd = gvk::context().create_window("Texture Cubemap");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->enable_resizing(true);
		mainWnd->set_additional_back_buffer_attachments({ 
			avk::attachment::declare(vk::Format::eD32Sfloat, avk::on_load::clear, avk::depth_stencil(), avk::on_store::dont_care)
		});
		mainWnd->set_presentaton_mode(gvk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = gvk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->add_queue_family_ownership(singleQueue);
		mainWnd->set_present_queue(singleQueue);
		
		// Create an instance of our main avk::element which contains all the functionality:
		auto app = texture_cubemap_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = gvk::imgui_manager(singleQueue);

		// GO:
		gvk::start(
			gvk::application_name("Gears-Vk + Auto-Vk Example: Texture Cubemap"),
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
