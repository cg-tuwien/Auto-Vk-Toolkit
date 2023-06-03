#include "imgui.h"

#include "camera_path.hpp"
#include "orbit_camera.hpp"
#include "configure_and_compose.hpp"
#include "imgui_manager.hpp"
#include "invokee.hpp"
#include "material_image_helpers.hpp"
#include "model.hpp"
#include "sequential_invoker.hpp"
#include "ui_helper.hpp"
#include "vk_convenience_functions.hpp"

class model_loader_app : public avk::invokee
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
	model_loader_app(avk::queue& aQueue)
		: mQueue{ &aQueue }
		, mScale{1.0f, 1.0f, 1.0f}
	{}

	void initialize() override
	{
		// use helper functions to create ImGui elements
		auto surfaceCap = avk::context().physical_device().getSurfaceCapabilitiesKHR(avk::context().main_window()->surface());
		mPresentationModeCombo = model_loader_ui_generator::get_presentation_mode_imgui_element();
		mSrgbFrameBufferCheckbox = model_loader_ui_generator::get_framebuffer_mode_imgui_element();
		mNumConcurrentFramesSlider = model_loader_ui_generator::get_number_of_concurrent_frames_imgui_element();
		mNumPresentableImagesSlider = model_loader_ui_generator::get_number_of_presentable_images_imgui_element(3, surfaceCap.minImageCount, surfaceCap.maxImageCount);
		mResizableWindowCheckbox = model_loader_ui_generator::get_window_resize_imgui_element();
		mAdditionalAttachmentsCheckbox = model_loader_ui_generator::get_additional_attachments_imgui_element();

		mInitTime = std::chrono::high_resolution_clock::now();

		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = avk::context().create_descriptor_cache();

		// Load a model from file:
		auto sponza = avk::model_t::load_from_file("assets/sponza_structure.obj", aiProcess_Triangulate | aiProcess_PreTransformVertices);
		// Get all the different materials of the model:
		auto distinctMaterials = sponza->distinct_material_configs();

		// The following might be a bit tedious still, but maybe it's not. For what it's worth, it is expressive.
		// The following loop gathers all the vertex and index data PER MATERIAL and constructs the buffers and materials.
		// Later, we'll use ONE draw call PER MATERIAL to draw the whole scene.
		std::vector<avk::material_config> allMatConfigs;
		for (const auto& pair : distinctMaterials) {
			auto& newElement = mDrawCalls.emplace_back();
			allMatConfigs.push_back(pair.first);
			newElement.mMaterialIndex = static_cast<int>(allMatConfigs.size() - 1);

			// 1. Gather all the vertex and index data from the sub meshes:
			for (auto index : pair.second) {
				avk::append_indices_and_vertex_data(
					avk::additional_index_data(	newElement.mIndices,	[&]() { return sponza->indices_for_mesh<uint32_t>(index);								} ),
					avk::additional_vertex_data(newElement.mPositions,	[&]() { return sponza->positions_for_mesh(index);							} ),
					avk::additional_vertex_data(newElement.mTexCoords,	[&]() { return sponza->texture_coordinates_for_mesh<glm::vec2>(index, 0);	} ),
					avk::additional_vertex_data(newElement.mNormals,	[&]() { return sponza->normals_for_mesh(index);								} )
				);
			}

			// 2. Build all the buffers for the GPU
			// 2.1 Positions:
			newElement.mPositionsBuffer = avk::context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_data(newElement.mPositions)
			);
			auto posFillCmd = newElement.mPositionsBuffer->fill(newElement.mPositions.data(), 0);

			// 2.2 Texture Coordinates:
			newElement.mTexCoordsBuffer = avk::context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_data(newElement.mTexCoords)
			);
			auto tcoFillCmd = newElement.mTexCoordsBuffer->fill(newElement.mTexCoords.data(), 0);

			// 2.3 Normals:
			newElement.mNormalsBuffer = avk::context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_data(newElement.mNormals)
			);
			auto nrmFillCmd = newElement.mNormalsBuffer->fill(newElement.mNormals.data(), 0);

			// 2.4 Indices:
			newElement.mIndexBuffer = avk::context().create_buffer(
				avk::memory_usage::device, {},
				avk::index_buffer_meta::create_from_data(newElement.mIndices)
			);
			auto idxFillCmd = newElement.mIndexBuffer->fill(newElement.mIndices.data(), 0);

			// Submit all the fill commands to the queue:
			auto fence = avk::context().record_and_submit_with_fence({
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
		auto [gpuMaterials, imageSamplers, materialCommands] = avk::convert_for_gpu_usage<avk::material_gpu_data>(
			allMatConfigs, false, true,
			avk::image_usage::general_texture,
			avk::filter_mode::trilinear
		);

		mImageSamplers = std::move(imageSamplers);

		// A buffer to hold all the material data:
		mMaterialBuffer = avk::context().create_buffer(
			avk::memory_usage::device, {},
			avk::storage_buffer_meta::create_from_data(gpuMaterials)
		);

		// Submit the commands material commands and the materials buffer fill to the device:
		auto matFence = avk::context().record_and_submit_with_fence({
			std::move(materialCommands),
			mMaterialBuffer->fill(gpuMaterials.data(), 0)
		}, *mQueue);
		matFence->wait_until_signalled();

		// Create a buffer for the transformation matrices in a host coherent memory region (one for each frame in flight):
		for (int i = 0; i < 10; ++i) { // Up to 10 concurrent frames can be configured through the UI.
			mViewProjBuffers[i] = avk::context().create_buffer(
				avk::memory_usage::host_coherent, {},
				avk::uniform_buffer_meta::create_from_data(glm::mat4())
			);
		}

		auto swapChainFormat = avk::context().main_window()->swap_chain_image_format();
		// Create our rasterization graphics pipeline with the required configuration:
		mPipeline = avk::context().create_graphics_pipeline_for(
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
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)),
			// We'll render to the back buffer, which has a color attachment always, and in our case additionally a depth
			// attachment, which has been configured when creating the window. See main() function!
			avk::context().create_renderpass({
				avk::attachment::declare(avk::format_from_window_color_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::color(0)     , avk::on_store::store),	 
				avk::attachment::declare(avk::format_from_window_depth_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::depth_stencil, avk::on_store::dont_care)
			}, avk::context().main_window()->renderpass_reference().subpass_dependencies()),
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

		mUpdater->on(avk::swapchain_resized_event(avk::context().main_window())).invoke([this]() {
			this->mQuakeCam.set_aspect_ratio(avk::context().main_window()->aspect_ratio());
			this->mOrbitCam.set_aspect_ratio(avk::context().main_window()->aspect_ratio());
		});

		//first make sure render pass is updated
		mUpdater->on(avk::swapchain_format_changed_event(avk::context().main_window()),
					 avk::swapchain_additional_attachments_changed_event(avk::context().main_window())
		).invoke([this]() {
			std::vector<avk::attachment> renderpassAttachments = {
				avk::attachment::declare(avk::format_from_window_color_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::color(0),		avk::on_store::store),	 // But not in presentable format, because ImGui comes after
			};
			if (mAdditionalAttachmentsCheckbox->checked())	{
				renderpassAttachments.push_back(avk::attachment::declare(avk::format_from_window_depth_buffer(avk::context().main_window()), avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::depth_stencil, avk::on_store::dont_care));
			}
			auto renderPass = avk::context().create_renderpass(renderpassAttachments, avk::context().main_window()->renderpass_reference().subpass_dependencies());
			avk::context().replace_render_pass_for_pipeline(mPipeline, std::move(renderPass));
		}).then_on( // ... next, at this point, we are sure that the render pass is correct -> check if there are events that would update the pipeline
			avk::swapchain_changed_event(avk::context().main_window()),
			avk::shader_files_changed_event(mPipeline.as_reference())
		).update(mPipeline);


		// Add the cameras to the composition (and let them handle updates)
		mOrbitCam.set_translation({ 0.0f, 0.0f, 0.0f });
		mQuakeCam.set_translation({ 0.0f, 0.0f, 0.0f });
		mOrbitCam.set_perspective_projection(glm::radians(60.0f), avk::context().main_window()->aspect_ratio(), 0.3f, 1000.0f);
		mQuakeCam.set_perspective_projection(glm::radians(60.0f), avk::context().main_window()->aspect_ratio(), 0.3f, 1000.0f);
		avk::current_composition()->add_element(mOrbitCam);
		avk::current_composition()->add_element(mQuakeCam);
		mQuakeCam.disable();

		auto imguiManager = avk::current_composition()->element_by_type<avk::imgui_manager>();
		if(nullptr != imguiManager) {
			imguiManager->add_callback([this, imguiManager] {
				bool isEnabled = this->is_enabled();
		        ImGui::Begin("Info & Settings");
				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);

				ImGui::Separator();
				bool quakeCamEnabled = mQuakeCam.is_enabled();
				if (ImGui::Checkbox("Enable Quake Camera", &quakeCamEnabled)) {
					if (quakeCamEnabled) { // => should be enabled
						mQuakeCam.set_matrix(mOrbitCam.matrix());
						mQuakeCam.enable();
						mOrbitCam.disable();
					}
				}
				if (quakeCamEnabled) {
				    ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), "[F1] to exit Quake Camera navigation.");
					if (avk::input().key_pressed(avk::key_code::f1)) {
						mOrbitCam.set_matrix(mQuakeCam.matrix());
						mOrbitCam.enable();
						mQuakeCam.disable();
					}
				}
				if (imguiManager->begin_wanting_to_occupy_mouse() && mOrbitCam.is_enabled()) {
					mOrbitCam.disable();
				}
				if (imguiManager->end_wanting_to_occupy_mouse() && !mQuakeCam.is_enabled()) {
					mOrbitCam.enable();
				}
				ImGui::Separator();

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
		auto mainWnd = avk::context().main_window();
		auto ifi = mainWnd->current_in_flight_index();

		auto viewProjMat = mQuakeCam.is_enabled()
		    ? mQuakeCam.projection_and_view_matrix()
		    : mOrbitCam.projection_and_view_matrix();
		auto emptyCmd = mViewProjBuffers[ifi]->fill(glm::value_ptr(viewProjMat), 0);
		
		// Get a command pool to allocate command buffers from:
		auto& commandPool = avk::context().get_command_pool_for_single_use_command_buffers(*mQueue);

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();
		
		// Create a command buffer and render into the *current* swap chain image:
		auto cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		
		avk::context().record({
				avk::command::render_pass(mPipeline->renderpass_reference(), avk::context().main_window()->current_backbuffer_reference(), {
					avk::command::bind_pipeline(mPipeline.as_reference()),
					avk::command::bind_descriptors(mPipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
						avk::descriptor_binding(0, 0, avk::as_combined_image_samplers(mImageSamplers, avk::layout::shader_read_only_optimal)),
						avk::descriptor_binding(0, 1, mViewProjBuffers[ifi]),
						avk::descriptor_binding(1, 0, mMaterialBuffer)
					})),

					// Draw all the draw calls:
					avk::command::custom_commands([&,this](avk::command_buffer_t& cb) { // If there is no avk::command::... struct for a particular command, we can always use avk::command::custom_commands
						for (auto& drawCall : mDrawCalls) {
							cb.record({
								// Set the push constants per draw call:
								avk::command::push_constants(
									mPipeline->layout(),
									transformation_matrices{
										// Set model matrix for this mesh:
										glm::scale(glm::vec3(0.01f) * mScale),
										// Set material index for this mesh:
										drawCall.mMaterialIndex
									}
								),

								// Make the draw call:
								avk::command::draw_indexed(
									// Bind and use the index buffer:
									drawCall.mIndexBuffer.as_reference(),
									// Bind the vertex input buffers in the right order (corresponding to the layout specifiers in the vertex shader)
									drawCall.mPositionsBuffer.as_reference(), drawCall.mTexCoordsBuffer.as_reference(), drawCall.mNormalsBuffer.as_reference()
								)
							});
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

		if (avk::input().key_pressed(avk::key_code::c)) {
			// Center the cursor:
			auto resolution = avk::context().main_window()->resolution();
			avk::context().main_window()->set_cursor_pos({ resolution[0] / 2.0, resolution[1] / 2.0 });
		}
		if (avk::input().key_pressed(avk::key_code::escape)) {
			// Stop the current composition:
			avk::current_composition()->stop();
		}
		if (avk::input().key_pressed(avk::key_code::left)) {
			mQuakeCam.look_along(avk::left());
		}
		if (avk::input().key_pressed(avk::key_code::right)) {
			mQuakeCam.look_along(avk::right());
		}
		if (avk::input().key_pressed(avk::key_code::up)) {
			mQuakeCam.look_along(avk::front());
		}
		if (avk::input().key_pressed(avk::key_code::down)) {
			mQuakeCam.look_along(avk::back());
		}
		if (avk::input().key_pressed(avk::key_code::page_up)) {
			mQuakeCam.look_along(avk::up());
		}
		if (avk::input().key_pressed(avk::key_code::page_down)) {
			mQuakeCam.look_along(avk::down());
		}
		if (avk::input().key_pressed(avk::key_code::home)) {
			mQuakeCam.look_at(glm::vec3{0.0f, 0.0f, 0.0f});
		}

		// Automatic camera path:
		if (avk::input().key_pressed(avk::key_code::c)) {
			if (avk::input().key_down(avk::key_code::left_shift)) { // => disable
				if (mCameraPath.has_value()) {
					avk::current_composition()->remove_element_immediately(mCameraPath.value());
					mCameraPath.reset();
				}
			}
			else { // => enable
				if (mCameraPath.has_value()) {
					avk::current_composition()->remove_element_immediately(mCameraPath.value());
				}
				mCameraPath.emplace(mQuakeCam);
				avk::current_composition()->add_element(mCameraPath.value());
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
    glm::vec3 mScale;

	avk::orbit_camera mOrbitCam;
	avk::quake_camera mQuakeCam;
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
		auto mainWnd = avk::context().create_window("Model Loader");

		mainWnd->set_resolution({ 1000, 480 });
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
		auto app = model_loader_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = avk::imgui_manager(singleQueue);

		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			avk::application_name("Auto-Vk-Toolkit Example: Model Loader"),
			[](avk::validation_layers& config) {
				config.enable_feature(vk::ValidationFeatureEnableEXT::eSynchronizationValidation);
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
