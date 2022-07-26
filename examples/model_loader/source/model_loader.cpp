#include <gvk.hpp>
#include <imgui.h>
#include "camera_path.hpp"
#include "ui_helper.hpp"

class static_meshlets_app : public gvk::invokee
{
	struct data_for_draw_call
	{
		std::vector<glm::vec3> mPositions;
		std::vector<glm::vec2> mTexCoords;
		std::vector<glm::vec3> mNormals;
		std::vector<uint32_t> mIndices;

		avk::buffer mPositionsBuffer;
		avk::buffer mTexCoordsBuffer;
		avk::buffer mNormalsBuffer;
		avk::buffer mIndexBuffer;

		int mMaterialIndex;
	};

	struct transformation_matrices {
		glm::mat4 mModelMatrix;
		int mMaterialIndex;
	};

public: // v== avk::invokee overrides which will be invoked by the framework ==v
	static_meshlets_app(avk::queue& aQueue)
		: mQueue{ &aQueue }
		, mScale{1.0f, 1.0f, 1.0f}
	{}

	void initialize() override
	{
		// use helper functions to create ImGui elements
		auto surfaceCap = gvk::context().physical_device().getSurfaceCapabilitiesKHR(gvk::context().main_window()->surface());
		mPresentationModeCombo = model_loader_ui_generator::get_presentation_mode_imgui_element();
		mSrgbFrameBufferCheckbox = model_loader_ui_generator::get_framebuffer_mode_imgui_element();
		mNumConcurrentFramesSlider = model_loader_ui_generator::get_number_of_concurrent_frames_imgui_element();
		mNumPresentableImagesSlider = model_loader_ui_generator::get_number_of_presentable_images_imgui_element(3, surfaceCap.minImageCount, surfaceCap.maxImageCount);
		mResizableWindowCheckbox = model_loader_ui_generator::get_window_resize_imgui_element();
		mAdditionalAttachmentsCheckbox = model_loader_ui_generator::get_additional_attachments_imgui_element();

		mInitTime = std::chrono::high_resolution_clock::now();

		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = gvk::context().create_descriptor_cache();

		// Load a model from file:
		auto sponza = gvk::model_t::load_from_file("assets/sponza_structure.obj", aiProcess_Triangulate | aiProcess_PreTransformVertices);
		// Get all the different materials of the model:
		auto distinctMaterials = sponza->distinct_material_configs();

		// The following might be a bit tedious still, but maybe it's not. For what it's worth, it is expressive.
		// The following loop gathers all the vertex and index data PER MATERIAL and constructs the buffers and materials.
		// Later, we'll use ONE draw call PER MATERIAL to draw the whole scene.
		std::vector<gvk::material_config> allMatConfigs;
		for (const auto& pair : distinctMaterials) {
			auto& newElement = mDrawCalls.emplace_back();
			allMatConfigs.push_back(pair.first);
			newElement.mMaterialIndex = static_cast<int>(allMatConfigs.size() - 1);

			// 1. Gather all the vertex and index data from the sub meshes:
			for (auto index : pair.second) {
				gvk::append_indices_and_vertex_data(
					gvk::additional_index_data(	newElement.mIndices,	[&]() { return sponza->indices_for_mesh<uint32_t>(index);								} ),
					gvk::additional_vertex_data(newElement.mPositions,	[&]() { return sponza->positions_for_mesh(index);							} ),
					gvk::additional_vertex_data(newElement.mTexCoords,	[&]() { return sponza->texture_coordinates_for_mesh<glm::vec2>(index, 0);	} ),
					gvk::additional_vertex_data(newElement.mNormals,	[&]() { return sponza->normals_for_mesh(index);								} )
				);
			}

			// 2. Build all the buffers for the GPU
			// 2.1 Positions:
			newElement.mPositionsBuffer = gvk::context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_data(newElement.mPositions)
			);
			auto posFillCmd = newElement.mPositionsBuffer->fill(newElement.mPositions.data(), 0);

			// 2.2 Texture Coordinates:
			newElement.mTexCoordsBuffer = gvk::context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_data(newElement.mTexCoords)
			);
			auto tcoFillCmd = newElement.mTexCoordsBuffer->fill(newElement.mTexCoords.data(), 0);

			// 2.3 Normals:
			newElement.mNormalsBuffer = gvk::context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_data(newElement.mNormals)
			);
			auto nrmFillCmd = newElement.mNormalsBuffer->fill(newElement.mNormals.data(), 0);

			// 2.4 Indices:
			newElement.mIndexBuffer = gvk::context().create_buffer(
				avk::memory_usage::device, {},
				avk::index_buffer_meta::create_from_data(newElement.mIndices)
			);
			auto idxFillCmd = newElement.mIndexBuffer->fill(newElement.mIndices.data(), 0);

			// Submit all the fill commands to the queue:
			auto fence = gvk::context().record_and_submit_with_fence({
				std::move(posFillCmd),
				std::move(tcoFillCmd),
				std::move(nrmFillCmd),
				std::move(idxFillCmd)
				// ^ No need for any synchronization in-between, because the commands do not depend on each other.
			}, *mQueue);
			// Wait on the host until the device is done:
			fence->wait_until_signalled();
		}

		// For all the different materials, transfer them in structs which are well
		// suited for GPU-usage (proper alignment, and containing only the relevant data),
		// also load all the referenced images from file and provide access to them
		// via samplers; It all happens in `ak::convert_for_gpu_usage`:
		auto [gpuMaterials, imageSamplers, materialCommands] = gvk::convert_for_gpu_usage<gvk::material_gpu_data>(
			allMatConfigs, false, true,
			avk::image_usage::general_texture,
			avk::filter_mode::trilinear
		);

		// A buffer to hold all the material data:
		mMaterialBuffer = gvk::context().create_buffer(
			avk::memory_usage::device, {},
			avk::storage_buffer_meta::create_from_data(gpuMaterials)
		);

		// Submit the commands necessary for that ^ instruction to the device:
		auto matFence = gvk::context().record_and_submit_with_fence({
			std::move(materialCommands),
			mMaterialBuffer->fill(gpuMaterials.data(), 0)
		}, *mQueue);
		matFence->wait_until_signalled();

		// Create a buffer for the transformation matrices in a host coherent memory region (one for each frame in flight):
		for (int i = 0; i < 10; ++i) { // Up to 10 concurrent frames can be configured through the UI.
			mViewProjBuffers[i] = gvk::context().create_buffer(
				avk::memory_usage::host_coherent, {},
				avk::uniform_buffer_meta::create_from_data(glm::mat4())
			);
		}

		mImageSamplers = std::move(imageSamplers);

		auto swapChainFormat = gvk::context().main_window()->swap_chain_image_format();
		// Create our rasterization graphics pipeline with the required configuration:
		mPipeline = gvk::context().create_graphics_pipeline_for(
			// Specify which shaders the pipeline consists of:
			avk::vertex_shader("shaders/transform_and_pass_pos_nrm_uv.vert"),
			avk::fragment_shader("shaders/diffuse_shading_fixed_lightsource.frag"),
			// The next 3 lines define the format and location of the vertex shader inputs:
			// (The dummy values (like glm::vec3) tell the pipeline the format of the respective input)
			avk::from_buffer_binding(0) -> stream_per_vertex<glm::vec3>() -> to_location(0), // <-- corresponds to vertex shader's inPosition
			avk::from_buffer_binding(1) -> stream_per_vertex<glm::vec2>() -> to_location(1), // <-- corresponds to vertex shader's inTexCoord
			avk::from_buffer_binding(2) -> stream_per_vertex<glm::vec3>() -> to_location(2), // <-- corresponds to vertex shader's inNormal
			// Some further settings:
			avk::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(gvk::context().main_window()->backbuffer_reference_at_index(0)),
			// We'll render to the back buffer, which has a color attachment always, and in our case additionally a depth
			// attachment, which has been configured when creating the window. See main() function!
			gvk::context().create_renderpass({
				avk::attachment::declare(gvk::format_from_window_color_buffer(gvk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::color(0)     , avk::on_store::store),	 
				avk::attachment::declare(gvk::format_from_window_depth_buffer(gvk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::depth_stencil, avk::on_store::dont_care)
			}, gvk::context().main_window()->renderpass_reference().subpass_dependencies()),
			// The following define additional data which we'll pass to the pipeline:
			//   We'll pass two matrices to our vertex shader via push constants:
			avk::push_constant_binding_data { avk::shader_type::vertex, 0, sizeof(transformation_matrices) },
			avk::descriptor_binding(0, 0, avk::as_combined_image_samplers(mImageSamplers, avk::layout::shader_read_only_optimal)),
			avk::descriptor_binding(0, 1, mViewProjBuffers[0]),
			avk::descriptor_binding(1, 0, mMaterialBuffer)
		);

		// set up updater
		// we want to use an updater, so create one:

		mUpdater.emplace();
		mPipeline.enable_shared_ownership(); // Make it usable with the updater

		mUpdater->on(gvk::swapchain_resized_event(gvk::context().main_window())).invoke([this]() {
			this->mQuakeCam.set_aspect_ratio(gvk::context().main_window()->aspect_ratio());
		});

		//first make sure render pass is updated
		mUpdater->on(gvk::swapchain_format_changed_event(gvk::context().main_window()),
					 gvk::swapchain_additional_attachments_changed_event(gvk::context().main_window())
		).invoke([this]() {
			std::vector<avk::attachment> renderpassAttachments = {
				avk::attachment::declare(gvk::format_from_window_color_buffer(gvk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::color(0),		avk::on_store::store),	 // But not in presentable format, because ImGui comes after
			};
			if (mAdditionalAttachmentsCheckbox->checked())	{
				renderpassAttachments.push_back(avk::attachment::declare(gvk::format_from_window_depth_buffer(gvk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::depth_stencil, avk::on_store::dont_care));
			}
			auto renderPass = gvk::context().create_renderpass(renderpassAttachments, gvk::context().main_window()->renderpass_reference().subpass_dependencies());
			gvk::context().replace_render_pass_for_pipeline(mPipeline, std::move(renderPass));
		}).then_on( // ... next, at this point, we are sure that the render pass is correct -> check if there are events that would update the pipeline
			gvk::swapchain_changed_event(gvk::context().main_window()),
			gvk::shader_files_changed_event(mPipeline.as_reference())
		).update(mPipeline);


		// Add the camera to the composition (and let it handle the updates)
		mQuakeCam.set_translation({ 0.0f, 0.0f, 0.0f });
		mQuakeCam.set_perspective_projection(glm::radians(60.0f), gvk::context().main_window()->aspect_ratio(), 0.3f, 1000.0f);
		//mQuakeCam.set_orthographic_projection(-5, 5, -5, 5, 0.5, 100);
		gvk::current_composition()->add_element(mQuakeCam);

		auto imguiManager = gvk::current_composition()->element_by_type<gvk::imgui_manager>();
		if(nullptr != imguiManager) {
			imguiManager->add_callback([this](){
				bool isEnabled = this->is_enabled();
		        ImGui::Begin("Info & Settings");
				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
				ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), "[F1]: Toggle input-mode");
				ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), " (UI vs. scene navigation)");
				ImGui::DragFloat3("Scale", glm::value_ptr(mScale), 0.005f, 0.01f, 10.0f);
				ImGui::Checkbox("Enable/Disable invokee", &isEnabled);
				if (isEnabled != this->is_enabled())
				{
					if (!isEnabled) this->disable();
					else this->enable();
				}

				mSrgbFrameBufferCheckbox->invokeImGui();
				mResizableWindowCheckbox->invokeImGui();
				mAdditionalAttachmentsCheckbox->invokeImGui();
				mNumConcurrentFramesSlider->invokeImGui();
				mNumPresentableImagesSlider->invokeImGui();
				mPresentationModeCombo->invokeImGui();

				ImGui::End();
			});
		}
	}

	void render() override
	{
		auto mainWnd = gvk::context().main_window();
		auto ifi = mainWnd->current_in_flight_index();

		auto viewProjMat = mQuakeCam.projection_matrix() * mQuakeCam.view_matrix();
		auto emptyCmd = mViewProjBuffers[ifi]->fill(glm::value_ptr(viewProjMat), 0);
		
		// Get a command pool to allocate command buffers from:
		auto& commandPool = gvk::context().get_command_pool_for_single_use_command_buffers(*mQueue);

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();
		
		// Create a command buffer and render into the *current* swap chain image:
		auto cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		
		gvk::context().record({
				avk::command::render_pass(mPipeline->renderpass_reference(), gvk::context().main_window()->current_backbuffer_reference(), {
					avk::command::bind_pipeline(mPipeline.as_reference()),
					avk::command::bind_descriptors(mPipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
						avk::descriptor_binding(0, 0, avk::as_combined_image_samplers(mImageSamplers, avk::layout::shader_read_only_optimal)),
						avk::descriptor_binding(0, 1, mViewProjBuffers[ifi]),
						avk::descriptor_binding(1, 0, mMaterialBuffer)
					})),

					// Draw all the draw calls:
					avk::command::custom_commands([&,this](avk::command_buffer_t& cb) { // If there is no avk::command::... struct for a particular command, we can always use avk::command::custom_commands
						for (auto& drawCall : mDrawCalls) {
							// Set the push constants per draw call:
							cb.record(
								avk::command::push_constants(
									mPipeline->layout(),
									transformation_matrices{
										// Set model matrix for this mesh:
										glm::scale(glm::vec3(0.01f) * mScale),
										// Set material index for this mesh:
										drawCall.mMaterialIndex
									}
								)
							);

							// Make the draw call:
							cb.record(avk::command::draw_indexed(
								// Bind and use the index buffer:
								drawCall.mIndexBuffer.as_reference(),
								// Bind the vertex input buffers in the right order (corresponding to the layout specifiers in the vertex shader)
								drawCall.mPositionsBuffer.as_reference(), drawCall.mTexCoordsBuffer.as_reference(), drawCall.mNormalsBuffer.as_reference()
							));
						}
					}),

				})
			})
			.into_command_buffer(cmdBfr)
			.then_submit_to(*mQueue)
			// Do not start to render before the image has become available:
			.waiting_for(imageAvailableSemaphore >> avk::stage::color_attachment_output)
			.submit();
					
		mainWnd->handle_lifetime(std::move(cmdBfr));
	}

	void update() override
	{
		static int counter = 0;
		if (++counter == 4) {
			auto current = std::chrono::high_resolution_clock::now();
			auto time_span = current - mInitTime;
			auto int_min = std::chrono::duration_cast<std::chrono::minutes>(time_span).count();
			auto int_sec = std::chrono::duration_cast<std::chrono::seconds>(time_span).count();
			auto fp_ms = std::chrono::duration<double, std::milli>(time_span).count();
			printf("Time from init to fourth frame: %d min, %lld sec %lf ms\n", int_min, int_sec - static_cast<decltype(int_sec)>(int_min) * 60, fp_ms - 1000.0 * int_sec);
		}

		if (gvk::input().key_pressed(gvk::key_code::c)) {
			// Center the cursor:
			auto resolution = gvk::context().main_window()->resolution();
			gvk::context().main_window()->set_cursor_pos({ resolution[0] / 2.0, resolution[1] / 2.0 });
		}
		if (gvk::input().key_pressed(gvk::key_code::escape)) {
			// Stop the current composition:
			gvk::current_composition()->stop();
		}
		if (gvk::input().key_pressed(gvk::key_code::left)) {
			mQuakeCam.look_along(gvk::left());
		}
		if (gvk::input().key_pressed(gvk::key_code::right)) {
			mQuakeCam.look_along(gvk::right());
		}
		if (gvk::input().key_pressed(gvk::key_code::up)) {
			mQuakeCam.look_along(gvk::front());
		}
		if (gvk::input().key_pressed(gvk::key_code::down)) {
			mQuakeCam.look_along(gvk::back());
		}
		if (gvk::input().key_pressed(gvk::key_code::page_up)) {
			mQuakeCam.look_along(gvk::up());
		}
		if (gvk::input().key_pressed(gvk::key_code::page_down)) {
			mQuakeCam.look_along(gvk::down());
		}
		if (gvk::input().key_pressed(gvk::key_code::home)) {
			mQuakeCam.look_at(glm::vec3{0.0f, 0.0f, 0.0f});
		}

		if (gvk::input().key_pressed(gvk::key_code::f1)) {
			auto imguiManager = gvk::current_composition()->element_by_type<gvk::imgui_manager>();
			if (mQuakeCam.is_enabled()) {
				mQuakeCam.disable();
				if (nullptr != imguiManager) { imguiManager->enable_user_interaction(true); }
			}
			else {
				mQuakeCam.enable();
				if (nullptr != imguiManager) { imguiManager->enable_user_interaction(false); }
			}
		}

		// Automatic camera path:
		if (gvk::input().key_pressed(gvk::key_code::c)) {
			if (gvk::input().key_down(gvk::key_code::left_shift)) { // => disable
				if (mCameraPath.has_value()) {
					gvk::current_composition()->remove_element_immediately(mCameraPath.value());
					mCameraPath.reset();
				}
			}
			else { // => enable
				if (mCameraPath.has_value()) {
					gvk::current_composition()->remove_element_immediately(mCameraPath.value());
				}
				mCameraPath.emplace(mQuakeCam);
				gvk::current_composition()->add_element(mCameraPath.value());
			}
		}
	}

private: // v== Member variables ==v

	std::chrono::high_resolution_clock::time_point mInitTime;

	avk::queue* mQueue;
	avk::descriptor_cache mDescriptorCache;

	std::array<avk::buffer, 10> mViewProjBuffers;
	avk::buffer mMaterialBuffer;
	std::vector<avk::image_sampler> mImageSamplers;

	std::vector<data_for_draw_call> mDrawCalls;
	avk::graphics_pipeline mPipeline;
	gvk::quake_camera mQuakeCam;

	glm::vec3 mScale;

	std::optional<camera_path> mCameraPath;

	// imgui elements
	std::optional<combo_box_container> mPresentationModeCombo;
	std::optional<check_box_container> mSrgbFrameBufferCheckbox;
	std::optional<slider_container<int>> mNumConcurrentFramesSlider;
	std::optional<slider_container<int>> mNumPresentableImagesSlider;
	std::optional<check_box_container> mResizableWindowCheckbox;
	std::optional<check_box_container> mAdditionalAttachmentsCheckbox;

}; // model_loader_app

int main() // <== Starting point ==
{
	int result = EXIT_FAILURE;
	try {
		// Create a window and open it
		auto mainWnd = gvk::context().create_window("Model Loader");

		mainWnd->set_resolution({ 1000, 480 });
		mainWnd->enable_resizing(true);
		mainWnd->set_additional_back_buffer_attachments({
			avk::attachment::declare(vk::Format::eD32Sfloat, avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::depth_stencil, avk::on_store::dont_care)
		});
		mainWnd->set_presentaton_mode(gvk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = gvk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->set_queue_family_ownership(singleQueue.family_index());
		mainWnd->set_present_queue(singleQueue);

		// Create an instance of our main avk::element which contains all the functionality:
		auto app = static_meshlets_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = gvk::imgui_manager(singleQueue);

		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			gvk::application_name("Auto-Vk-Toolkit Example: Model Loader"),
			[](gvk::validation_layers& config) {
				config.enable_feature(vk::ValidationFeatureEnableEXT::eSynchronizationValidation);
			},
			// Pass windows:
			mainWnd,
			// Pass invokees:
			app, ui
		);

		// Create an invoker object, which defines the way how invokees/elements are invoked
		// (In this case, just sequentially in their execution order):
		gvk::sequential_invoker invoker;

		// With everything configured, let us start our render loop:
		composition.start_render_loop(
			// Callback in the case of update:
			[&invoker](const std::vector<gvk::invokee*>& aToBeInvoked) {
				// Call all the update() callbacks:
				invoker.invoke_updates(aToBeInvoked);
			},
			// Callback in the case of render:
			[&invoker](const std::vector<gvk::invokee*>& aToBeInvoked) {
				// Sync (wait for fences and so) per window BEFORE executing render callbacks
				gvk::context().execute_for_each_window([](gvk::window* wnd) {
					wnd->sync_before_render();
				});

				// Call all the render() callbacks:
				invoker.invoke_renders(aToBeInvoked);

				// Render per window:
				gvk::context().execute_for_each_window([](gvk::window* wnd) {
					wnd->render_frame();
				});
			}
		); // This is a blocking call, which loops until gvk::current_composition()->stop(); has been called (see update())
	
		result = EXIT_SUCCESS;
	}
	catch (gvk::logic_error&) {}
	catch (gvk::runtime_error&) {}
	catch (avk::logic_error&) {}
	catch (avk::runtime_error&) {}
	return result;
}
