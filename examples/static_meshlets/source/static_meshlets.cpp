#include <gvk.hpp>
#include <imgui.h>
#include "camera_path.hpp"
#include "ui_helper.hpp"
#include "../shaders/cpu_gpu_shared_config.h"

class model_loader_app : public gvk::invokee
{
	struct alignas(16) push_constants
	{
		uint32_t mHighlightMeshlets;
	};

	struct data_for_draw_call
	{
		avk::buffer mPositionsBuffer;
		avk::buffer mTexCoordsBuffer;
		avk::buffer mNormalsBuffer;
		avk::buffer mIndexBuffer;
#if !USE_DIRECT_MESHLET
		avk::buffer mMeshletDataBuffer;
#endif

		glm::mat4 mModelMatrix;

		int32_t mMaterialIndex;
	};

	struct loaded_data_for_draw_call
	{
		std::vector<glm::vec3> mPositions;
		std::vector<glm::vec2> mTexCoords;
		std::vector<glm::vec3> mNormals;
		std::vector<uint32_t> mIndices;
#if !USE_DIRECT_MESHLET
		std::vector<uint32_t> mMeshletData;
#endif

		glm::mat4 mModelMatrix;

		int32_t mMaterialIndex;
	};

	struct alignas(16) meshlet
	{
		glm::mat4 mTransformationMatrix;
		uint32_t mMaterialIndex;
		uint32_t mTexelBufferIndex;

#if USE_DIRECT_MESHLET
		gvk::meshlet_gpu_data mGeometry;
#else
		gvk::meshlet_indirect_gpu_data mGeometry;
#endif
	};

public: // v== avk::invokee overrides which will be invoked by the framework ==v
	model_loader_app(avk::queue& aQueue)
		: mQueue{ &aQueue }
		, mScale{ 1.0f, 1.0f, 1.0f }
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

		glm::mat4 globalTransform = glm::scale(glm::vec3(0.01f));
		std::vector<gvk::model> loadedModels;
		// Load a model from file:
		auto sponza = gvk::model_t::load_from_file("assets/sponza_structure.obj", aiProcess_Triangulate | aiProcess_PreTransformVertices);

		loadedModels.push_back(std::move(sponza));


		std::vector<gvk::material_config> rAllMatConfigs; // <-- Gather the material config from all models to be loaded
		std::vector<std::vector<glm::mat4>> rMeshRootMatricesPerModel;
		std::vector<loaded_data_for_draw_call> rDataForDrawCallOpaque;
		std::vector<meshlet> rMeshletsOpaqueGeometry;


		for (size_t i = 0; i < loadedModels.size(); ++i) {
			auto curModel = std::move(loadedModels[i]);

			const auto meshIndicesInOrder = curModel->select_all_meshes();

			auto distinctMaterials = curModel->distinct_material_configs();
			const auto matOffset = rAllMatConfigs.size();
			for (auto& pair : distinctMaterials) {
				rAllMatConfigs.push_back(pair.first);
			}

			for (size_t mpos = 0; mpos < meshIndicesInOrder.size(); mpos++) {
				auto meshIndex = meshIndicesInOrder[mpos];
				std::string meshname = curModel->name_of_mesh(mpos);

				auto texelBufferIndex = rDataForDrawCallOpaque.size();
				auto& drawCallData = rDataForDrawCallOpaque.emplace_back();

				drawCallData.mMaterialIndex = static_cast<int32_t>(matOffset);
				drawCallData.mModelMatrix = globalTransform * curModel->transformation_matrix_for_mesh(meshIndex);
				// Find and assign the correct material (in the ~"global" allMatConfigs vector!)
				for (auto pair : distinctMaterials) {
					if (std::end(pair.second) != std::find(std::begin(pair.second), std::end(pair.second), meshIndex)) {
						break;
					}

					drawCallData.mMaterialIndex++;
				}

				auto selection = gvk::make_models_and_meshes_selection(curModel, meshIndex);
				std::vector<gvk::mesh_index_t> meshIndices;
				meshIndices.push_back(meshIndex);
				// Build meshlets:
				std::tie(drawCallData.mPositions, drawCallData.mIndices) = gvk::get_vertices_and_indices(selection);
				drawCallData.mNormals = gvk::get_normals(selection);
				drawCallData.mTexCoords = gvk::get_2d_texture_coordinates(selection, 0);

				std::vector<std::tuple<avk::resource_ownership<gvk::model_t>, std::vector<gvk::mesh_index_t>>> meshletSelection;
				meshletSelection.emplace_back(avk::shared(curModel), meshIndices);

				auto cpuMeshlets = gvk::divide_into_meshlets(meshletSelection);
#if USE_DIRECT_MESHLET
				gvk::serializer serializer("direct_meshlets-"+meshname+"-"+std::to_string(mpos)+".cache");
				auto [gpuMeshlets, _] = gvk::convert_for_gpu_usage_cached<gvk::meshlet_gpu_data>(serializer, cpuMeshlets);
#else
				gvk::serializer serializer("indirect_meshlets-"+meshname+"-"+std::to_string(mpos)+".cache");
				auto [gpuMeshlets,generatedMeshletData] = gvk::convert_for_gpu_usage_cached<gvk::meshlet_indirect_gpu_data>(serializer, cpuMeshlets);
				drawCallData.mMeshletData = std::move(generatedMeshletData.value());
#endif

				serializer.flush();

				for (size_t mshltidx = 0; mshltidx < gpuMeshlets.size(); ++mshltidx) {
					auto& genMeshlet = gpuMeshlets[mshltidx];

					auto& ml = rMeshletsOpaqueGeometry.emplace_back();
					memset(&ml, 0, sizeof(meshlet));

#pragma region start to assemble meshlet struct
					ml.mTransformationMatrix = drawCallData.mModelMatrix;
					ml.mMaterialIndex = drawCallData.mMaterialIndex;
					ml.mTexelBufferIndex = static_cast<uint32_t>(texelBufferIndex);

#if USE_DIRECT_MESHLET
					memcpy(&ml.mGeometry, &genMeshlet, sizeof(gvk::meshlet_gpu_data));
#else
					memcpy(&ml.mGeometry, &genMeshlet, sizeof(gvk::meshlet_indirect_gpu_data));
#endif

#pragma endregion 
				}
			}
		} // end for (auto& loadInfo : toLoad)

		auto addDrawCalls = [this](auto& dataForDrawCall, auto& drawCallsTarget) {
			for (auto& drawCallData : dataForDrawCall) {
				auto& drawCall = drawCallsTarget.emplace_back();
				drawCall.mModelMatrix = drawCallData.mModelMatrix;
				drawCall.mMaterialIndex = drawCallData.mMaterialIndex;

				drawCall.mPositionsBuffer = gvk::context().create_buffer(avk::memory_usage::device, {},
					avk::vertex_buffer_meta::create_from_data(drawCallData.mPositions).describe_only_member(drawCallData.mPositions[0], avk::content_description::position),
					avk::storage_buffer_meta::create_from_data(drawCallData.mPositions),
					avk::uniform_texel_buffer_meta::create_from_data(drawCallData.mPositions).describe_only_member(drawCallData.mPositions[0]) // just take the vec3 as it is
				);

				drawCall.mIndexBuffer = gvk::context().create_buffer(avk::memory_usage::device, {},
					avk::index_buffer_meta::create_from_data(drawCallData.mIndices),
					avk::storage_buffer_meta::create_from_data(drawCallData.mIndices),
					avk::uniform_texel_buffer_meta::create_from_data(drawCallData.mIndices).template set_format<glm::uvec3>() // Set a diferent format => combine 3 consecutive elements into one unit
				);

				drawCall.mNormalsBuffer = gvk::context().create_buffer(avk::memory_usage::device, {},
					avk::vertex_buffer_meta::create_from_data(drawCallData.mNormals),
					avk::storage_buffer_meta::create_from_data(drawCallData.mNormals),
					avk::uniform_texel_buffer_meta::create_from_data(drawCallData.mNormals).describe_only_member(drawCallData.mNormals[0]) // just take the vec3 as it is
				);

				drawCall.mTexCoordsBuffer = gvk::context().create_buffer(avk::memory_usage::device, {},
					avk::vertex_buffer_meta::create_from_data(drawCallData.mTexCoords),
					avk::storage_buffer_meta::create_from_data(drawCallData.mTexCoords),
					avk::uniform_texel_buffer_meta::create_from_data(drawCallData.mTexCoords).describe_only_member(drawCallData.mTexCoords[0]) // just take the vec2 as it is   
				);

#if !USE_DIRECT_MESHLET
				drawCall.mMeshletDataBuffer = gvk::context().create_buffer(avk::memory_usage::device, {},
					avk::vertex_buffer_meta::create_from_data(drawCallData.mMeshletData),
					avk::storage_buffer_meta::create_from_data(drawCallData.mMeshletData),
					avk::uniform_texel_buffer_meta::create_from_data(drawCallData.mMeshletData).describe_only_member(drawCallData.mMeshletData[0])
				);
#endif

				drawCall.mPositionsBuffer->fill(drawCallData.mPositions.data(), 0, avk::sync::wait_idle(true));
				drawCall.mIndexBuffer->fill(drawCallData.mIndices.data(), 0, avk::sync::wait_idle(true));
				drawCall.mNormalsBuffer->fill(drawCallData.mNormals.data(), 0, avk::sync::wait_idle(true));
				drawCall.mTexCoordsBuffer->fill(drawCallData.mTexCoords.data(), 0, avk::sync::wait_idle(true));
#if !USE_DIRECT_MESHLET
				drawCall.mMeshletDataBuffer->fill(drawCallData.mMeshletData.data(), 0, avk::sync::wait_idle(true));
#endif

				mPositionBuffers.push_back(gvk::context().create_buffer_view(shared(drawCall.mPositionsBuffer)));
				mIndexBuffers.push_back(gvk::context().create_buffer_view(shared(drawCall.mIndexBuffer)));
				mNormalBuffers.push_back(gvk::context().create_buffer_view(shared(drawCall.mNormalsBuffer)));
				mTexCoordsBuffers.push_back(gvk::context().create_buffer_view(shared(drawCall.mTexCoordsBuffer)));
#if !USE_DIRECT_MESHLET
				mMeshletDataBuffers.push_back(gvk::context().create_buffer_view(shared(drawCall.mMeshletDataBuffer)));
#endif
			}
		};
		addDrawCalls(rDataForDrawCallOpaque, mDrawCalls);

		// Put the meshlets that we have gathered into a buffer:
		mMeshletsBuffer = gvk::context().create_buffer(
			avk::memory_usage::device, {},
			avk::storage_buffer_meta::create_from_data(rMeshletsOpaqueGeometry)
		);
		mMeshletsBuffer->fill(rMeshletsOpaqueGeometry.data(), 0, avk::sync::wait_idle(true));
		mNumMeshletWorkgroups = rMeshletsOpaqueGeometry.size();

		// For all the different materials, transfer them in structs which are well
		// suited for GPU-usage (proper alignment, and containing only the relevant data),
		// also load all the referenced images from file and provide access to them
		// via samplers; It all happens in `ak::convert_for_gpu_usage`:
		auto [gpuMaterials, imageSamplers] = gvk::convert_for_gpu_usage<gvk::material_gpu_data>(
			rAllMatConfigs, false, true,
			avk::image_usage::general_texture,
			avk::filter_mode::trilinear,
			avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler())
			);

		mViewProjBuffer = gvk::context().create_buffer(
			avk::memory_usage::host_visible, {},
			avk::uniform_buffer_meta::create_from_data(glm::mat4())
		);

		mMaterialBuffer = gvk::context().create_buffer(
			avk::memory_usage::host_visible, {},
			avk::storage_buffer_meta::create_from_data(gpuMaterials)
		);
		mMaterialBuffer->fill(
			gpuMaterials.data(), 0,
			avk::sync::not_required()
		);

		mImageSamplers = std::move(imageSamplers);

		auto swapChainFormat = gvk::context().main_window()->swap_chain_image_format();
		// Create our rasterization graphics pipeline with the required configuration:
		mPipeline = gvk::context().create_graphics_pipeline_for(
			// Specify which shaders the pipeline consists of:
			avk::task_shader("shaders/meshlet.task"),
			avk::mesh_shader("shaders/meshlet.mesh"),
			avk::fragment_shader("shaders/diffuse_shading_fixed_lightsource.frag"),
			// Some further settings:
			avk::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(gvk::context().main_window()->backbuffer_at_index(0)),
			// We'll render to the back buffer, which has a color attachment always, and in our case additionally a depth
			// attachment, which has been configured when creating the window. See main() function!
			avk::attachment::declare(gvk::format_from_window_color_buffer(gvk::context().main_window()), avk::on_load::clear, avk::color(0), avk::on_store::store),	 // But not in presentable format, because ImGui comes after
			avk::attachment::declare(gvk::format_from_window_depth_buffer(gvk::context().main_window()), avk::on_load::clear, avk::depth_stencil(), avk::on_store::dont_care),
			// The following define additional data which we'll pass to the pipeline:
			avk::push_constant_binding_data {avk::shader_type::fragment, 0, sizeof(push_constants)},
			avk::descriptor_binding(0, 0, mImageSamplers),
			avk::descriptor_binding(0, 1, mViewProjBuffer),
			avk::descriptor_binding(1, 0, mMaterialBuffer),
			avk::descriptor_binding(3, 0, avk::as_uniform_texel_buffer_views(mPositionBuffers)),
			avk::descriptor_binding(3, 1, avk::as_uniform_texel_buffer_views(mIndexBuffers)),
			avk::descriptor_binding(3, 2, avk::as_uniform_texel_buffer_views(mNormalBuffers)),
			avk::descriptor_binding(3, 3, avk::as_uniform_texel_buffer_views(mTexCoordsBuffers)),
#if !USE_DIRECT_MESHLET
			avk::descriptor_binding(3, 4, avk::as_uniform_texel_buffer_views(mMeshletDataBuffers)),
#endif
			avk::descriptor_binding(4, 0, mMeshletsBuffer)
		);
		// set up updater
		// we want to use an updater, so create one:

		mUpdater.emplace();
		mPipeline.enable_shared_ownership(); // Make it usable with the updater
		mUpdater->on(gvk::shader_files_changed_event(mPipeline)).update(mPipeline);
		mPipeline.enable_shared_ownership(); // Make it usable with the updater

		mUpdater->on(gvk::swapchain_resized_event(gvk::context().main_window())).invoke([this]() {
			this->mQuakeCam.set_aspect_ratio(gvk::context().main_window()->aspect_ratio());
			});

		//first make sure render pass is updated
		mUpdater->on(gvk::swapchain_format_changed_event(gvk::context().main_window()),
			gvk::swapchain_additional_attachments_changed_event(gvk::context().main_window())
		).invoke([this]() {
			std::vector<avk::attachment> renderpassAttachments = {
				avk::attachment::declare(gvk::format_from_window_color_buffer(gvk::context().main_window()), avk::on_load::clear, avk::color(0),		avk::on_store::store),	 // But not in presentable format, because ImGui comes after
			};
			if (mAdditionalAttachmentsCheckbox->checked()) {
				renderpassAttachments.push_back(avk::attachment::declare(gvk::format_from_window_depth_buffer(gvk::context().main_window()), avk::on_load::clear, avk::depth_stencil(), avk::on_store::dont_care));
			}
			auto renderPass = gvk::context().create_renderpass(renderpassAttachments);
			gvk::context().replace_render_pass_for_pipeline(mPipeline, std::move(renderPass));
			}).then_on( // ... next, at this point, we are sure that the render pass is correct -> check if there are events that would update the pipeline
				gvk::swapchain_changed_event(gvk::context().main_window()),
				gvk::shader_files_changed_event(mPipeline)
			).update(mPipeline);

			// Add the camera to the composition (and let it handle the updates)
			mQuakeCam.set_translation({ 0.0f, 0.0f, 0.0f });
			mQuakeCam.set_perspective_projection(glm::radians(60.0f), gvk::context().main_window()->aspect_ratio(), 0.3f, 1000.0f);
			//mQuakeCam.set_orthographic_projection(-5, 5, -5, 5, 0.5, 100);
			gvk::current_composition()->add_element(mQuakeCam);

			auto imguiManager = gvk::current_composition()->element_by_type<gvk::imgui_manager>();
			if (nullptr != imguiManager) {
				imguiManager->add_callback([this]() {
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

					ImGui::Checkbox("Highlight Meshlets", &mHighlightMeshlets);

					ImGui::End();
					});
			}
	}

	void render() override
	{
		auto mainWnd = gvk::context().main_window();

		auto viewProjMat = mQuakeCam.projection_matrix() * mQuakeCam.view_matrix();
		mViewProjBuffer->fill(glm::value_ptr(viewProjMat), 0, avk::sync::not_required());

		auto pushConstants = push_constants{ mHighlightMeshlets };

		auto& commandPool = gvk::context().get_command_pool_for_single_use_command_buffers(*mQueue);
		auto cmdbfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		cmdbfr->begin_recording();
		cmdbfr->begin_render_pass_for_framebuffer(mPipeline->get_renderpass(), gvk::context().main_window()->current_backbuffer());
		cmdbfr->bind_pipeline(avk::const_referenced(mPipeline));
		cmdbfr->bind_descriptors(mPipeline->layout(), mDescriptorCache.get_or_create_descriptor_sets({
			avk::descriptor_binding(0, 0, mImageSamplers),
			avk::descriptor_binding(0, 1, mViewProjBuffer),
			avk::descriptor_binding(1, 0, mMaterialBuffer),
			avk::descriptor_binding(3, 0, avk::as_uniform_texel_buffer_views(mPositionBuffers)),
			avk::descriptor_binding(3, 1, avk::as_uniform_texel_buffer_views(mIndexBuffers)),
			avk::descriptor_binding(3, 2, avk::as_uniform_texel_buffer_views(mNormalBuffers)),
			avk::descriptor_binding(3, 3, avk::as_uniform_texel_buffer_views(mTexCoordsBuffers)),
#if !USE_DIRECT_MESHLET
					avk::descriptor_binding(3, 4, avk::as_uniform_texel_buffer_views(mMeshletDataBuffers)),
#endif
						avk::descriptor_binding(4, 0, mMeshletsBuffer)
			}));
		cmdbfr->handle().pushConstants(mPipeline->layout_handle(), vk::ShaderStageFlagBits::eFragment, 0u, sizeof(push_constants), &pushConstants);
		cmdbfr->handle().drawMeshTasksNV(mNumMeshletWorkgroups, 0, gvk::context().dynamic_dispatch());

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
			mQuakeCam.look_at(glm::vec3{ 0.0f, 0.0f, 0.0f });
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

	avk::buffer mViewProjBuffer;
	avk::buffer mMaterialBuffer;
	avk::buffer mMeshletsBuffer;
	std::vector<avk::image_sampler> mImageSamplers;

	std::vector<data_for_draw_call> mDrawCalls;
	avk::graphics_pipeline mPipeline;
	gvk::quake_camera mQuakeCam;
	size_t mNumMeshletWorkgroups;

	glm::vec3 mScale;

	std::optional<camera_path> mCameraPath;

	std::vector<avk::buffer_view> mPositionBuffers;
	std::vector<avk::buffer_view> mIndexBuffers;
	std::vector<avk::buffer_view> mTexCoordsBuffers;
	std::vector<avk::buffer_view> mNormalBuffers;
#if !USE_DIRECT_MESHLET
	std::vector<avk::buffer_view> mMeshletDataBuffers;
#endif

	bool mHighlightMeshlets;

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
	try {
		// Create a window and open it
		auto mainWnd = gvk::context().create_window("Static Meshlets");

		mainWnd->set_resolution({ 1000, 480 });
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
		auto app = model_loader_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = gvk::imgui_manager(singleQueue);

		// GO:
		gvk::start(
			gvk::application_name("Gears-Vk + Auto-Vk Example: Static Meshlets"),
			gvk::required_device_extensions(VK_NV_MESH_SHADER_EXTENSION_NAME)
			.add_extension(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME),
			[](vk::PhysicalDeviceVulkan12Features& features) {
				features.setUniformAndStorageBuffer8BitAccess(VK_TRUE);
				features.setStorageBuffer8BitAccess(VK_TRUE);
			},
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