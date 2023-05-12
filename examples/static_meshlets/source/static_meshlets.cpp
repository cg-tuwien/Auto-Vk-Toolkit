#include <ranges>
#include <imgui.h>

#include "configure_and_compose.hpp"
#include "imgui_manager.hpp"
#include "invokee.hpp"
#include "material_image_helpers.hpp"
#include "meshlet_helpers.hpp"
#include "model.hpp"
#include "serializer.hpp"
#include "sequential_invoker.hpp"
#include "quake_camera.hpp"
#include "vk_convenience_functions.hpp"

class static_meshlets_app : public avk::invokee
{
	static constexpr size_t sNumVertices = 64;
	static constexpr size_t sNumIndices = 378;

	struct alignas(16) push_constants
	{
		vk::Bool32 mHighlightMeshlets;
		int32_t    mVisibleMeshletIndexFrom;
		int32_t    mVisibleMeshletIndexTo;
	};

	/** Contains the necessary buffers for drawing everything */
	struct data_for_draw_call
	{
		avk::buffer mPositionsBuffer;
		avk::buffer mTexCoordsBuffer;
		avk::buffer mNormalsBuffer;

		glm::mat4 mModelMatrix;

		int32_t mMaterialIndex;
	};

	/** Contains the data for each draw call */
	struct loaded_data_for_draw_call
	{
		std::vector<glm::vec3> mPositions;
		std::vector<glm::vec2> mTexCoords;
		std::vector<glm::vec3> mNormals;
		std::vector<uint32_t> mIndices;

		glm::mat4 mModelMatrix;

		int32_t mMaterialIndex;
	};

	/** The meshlet we upload to the gpu with its additional data. */
	struct alignas(16) meshlet
	{
		glm::mat4 mTransformationMatrix;
		uint32_t mMaterialIndex;
		uint32_t mTexelBufferIndex;

		avk::meshlet_gpu_data<sNumVertices, sNumIndices> mGeometry;
	};

public: // v== avk::invokee overrides which will be invoked by the framework ==v
	static_meshlets_app(avk::queue& aQueue)
		: mQueue{ &aQueue }
	{}

	/** Creates buffers for all the drawcalls.
	 *  Called after everything has been loaded and split into meshlets properly.
	 *  @param dataForDrawCall		The loaded data for the drawcalls.
	 *	@param drawCallsTarget		The target vector for the draw call data.
	 */
	void add_draw_calls(std::vector<loaded_data_for_draw_call>& dataForDrawCall, std::vector<data_for_draw_call>& drawCallsTarget) {
		for (auto& drawCallData : dataForDrawCall) {
			auto& drawCall = drawCallsTarget.emplace_back();
			drawCall.mModelMatrix = drawCallData.mModelMatrix;
			drawCall.mMaterialIndex = drawCallData.mMaterialIndex;

			drawCall.mPositionsBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_data(drawCallData.mPositions).describe_only_member(drawCallData.mPositions[0], avk::content_description::position),
				avk::storage_buffer_meta::create_from_data(drawCallData.mPositions),
				avk::uniform_texel_buffer_meta::create_from_data(drawCallData.mPositions).describe_only_member(drawCallData.mPositions[0]) // just take the vec3 as it is
			);

			drawCall.mNormalsBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_data(drawCallData.mNormals),
				avk::storage_buffer_meta::create_from_data(drawCallData.mNormals),
				avk::uniform_texel_buffer_meta::create_from_data(drawCallData.mNormals).describe_only_member(drawCallData.mNormals[0]) // just take the vec3 as it is
			);

			drawCall.mTexCoordsBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_data(drawCallData.mTexCoords),
				avk::storage_buffer_meta::create_from_data(drawCallData.mTexCoords),
				avk::uniform_texel_buffer_meta::create_from_data(drawCallData.mTexCoords).describe_only_member(drawCallData.mTexCoords[0]) // just take the vec2 as it is   
			);

			avk::context().record_and_submit_with_fence({
				drawCall.mPositionsBuffer->fill(drawCallData.mPositions.data(), 0),
				drawCall.mNormalsBuffer->fill(drawCallData.mNormals.data(), 0),
				drawCall.mTexCoordsBuffer->fill(drawCallData.mTexCoords.data(), 0)
			}, *mQueue)->wait_until_signalled();

			// add them to the texel buffers
			mPositionBuffers.push_back(avk::context().create_buffer_view(drawCall.mPositionsBuffer));
			mNormalBuffers.push_back(avk::context().create_buffer_view(drawCall.mNormalsBuffer));
			mTexCoordsBuffers.push_back(avk::context().create_buffer_view(drawCall.mTexCoordsBuffer));
		}
	}

	void initialize() override
	{
		// use helper functions to create ImGui elements
		auto surfaceCap = avk::context().physical_device().getSurfaceCapabilitiesKHR(avk::context().main_window()->surface());

		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = avk::context().create_descriptor_cache();

		glm::mat4 globalTransform = glm::translate(glm::vec3(0.f, -0.5f, -3.f)) * glm::scale(glm::vec3(1.f));
		std::vector<avk::model> loadedModels;
		// Load a model from file:
		auto bunny = avk::model_t::load_from_file("assets/stanford_bunny.obj", aiProcess_Triangulate | aiProcess_PreTransformVertices);

		loadedModels.push_back(std::move(bunny));

		std::vector<avk::material_config> allMatConfigs; // <-- Gather the material config from all models to be loaded
		std::vector<std::vector<glm::mat4>> meshRootMatricesPerModel;
		std::vector<loaded_data_for_draw_call> dataForDrawCall;
		std::vector<meshlet> meshletsGeometry;

		// Generate the meshlets for each loaded model.
		for (size_t i = 0; i < loadedModels.size(); ++i) {
			auto& curModel = loadedModels[i];

			// get all the meshlet indices of the model
			const auto meshIndicesInOrder = curModel->select_all_meshes();

			auto distinctMaterials = curModel->distinct_material_configs();
			const auto matOffset = allMatConfigs.size();
			// add all the materials of the model
			for (auto& pair : distinctMaterials) {
				allMatConfigs.push_back(pair.first);
			}

			// Generate meshlets for each submesh of the current loaded model. Load all it's data into the draw call for later use.
			for (size_t mpos = 0; mpos < meshIndicesInOrder.size(); mpos++) {
				auto meshIndex = meshIndicesInOrder[mpos];
				std::string meshname = curModel->name_of_mesh(mpos);

				auto texelBufferIndex = dataForDrawCall.size();
				auto& drawCallData = dataForDrawCall.emplace_back();

				drawCallData.mMaterialIndex = static_cast<int32_t>(matOffset);
				drawCallData.mModelMatrix = globalTransform * curModel->transformation_matrix_for_mesh(meshIndex);
				// Find and assign the correct material in the allMatConfigs vector
				for (auto pair : distinctMaterials) {
					if (std::end(pair.second) != std::ranges::find(pair.second, meshIndex)) {
						break;
					}
					drawCallData.mMaterialIndex++;
				}

				auto selection = avk::make_model_references_and_mesh_indices_selection(curModel, meshIndex);
				// Build meshlets:
				std::tie(drawCallData.mPositions, drawCallData.mIndices) = avk::get_vertices_and_indices(selection);
				drawCallData.mNormals = avk::get_normals(selection);
				drawCallData.mTexCoords = avk::get_2d_texture_coordinates(selection, 0);

				// create selection for the meshlets
				auto meshletSelection = avk::make_models_and_mesh_indices_selection(curModel, meshIndex);

				auto cpuMeshlets = avk::divide_into_meshlets(meshletSelection);
				avk::serializer serializer("direct_meshlets-" + meshname + "-" + std::to_string(mpos) + ".cache");
				auto [gpuMeshlets, _] = avk::convert_for_gpu_usage_cached<avk::meshlet_gpu_data<sNumVertices, sNumIndices>>(serializer, cpuMeshlets);

				serializer.flush();

				// fill our own meshlets with the loaded/generated data
				for (size_t mshltidx = 0; mshltidx < gpuMeshlets.size(); ++mshltidx) {
					auto& genMeshlet = gpuMeshlets[mshltidx];

					auto& ml = meshletsGeometry.emplace_back(meshlet{});

#pragma region start to assemble meshlet struct
					ml.mTransformationMatrix = drawCallData.mModelMatrix;
					ml.mMaterialIndex = drawCallData.mMaterialIndex;
					ml.mTexelBufferIndex = static_cast<uint32_t>(texelBufferIndex);

					ml.mGeometry = genMeshlet;
#pragma endregion 
				}
			}
		} // for (size_t i = 0; i < loadedModels.size(); ++i)

		// create all the buffers for our drawcall data
		add_draw_calls(dataForDrawCall, mDrawCalls);

		// Put the meshlets that we have gathered into a buffer:
		mMeshletsBuffer = avk::context().create_buffer(
			avk::memory_usage::device, {},
			avk::storage_buffer_meta::create_from_data(meshletsGeometry)
		);
		mNumMeshlets = static_cast<uint32_t>(meshletsGeometry.size());
		mShowMeshletsTo = static_cast<int>(mNumMeshlets);

		// For all the different materials, transfer them in structs which are well
		// suited for GPU-usage (proper alignment, and containing only the relevant data),
		// also load all the referenced images from file and provide access to them
		// via samplers; It all happens in `ak::convert_for_gpu_usage`:
		auto [gpuMaterials, imageSamplers, matCommands] = avk::convert_for_gpu_usage<avk::material_gpu_data>(
			allMatConfigs, false, false,
			avk::image_usage::general_texture,
			avk::filter_mode::trilinear
		);

		avk::context().record_and_submit_with_fence({
			mMeshletsBuffer->fill(meshletsGeometry.data(), 0),
			matCommands
		}, *mQueue)->wait_until_signalled();

		// One for each concurrent frame
		const auto concurrentFrames = avk::context().main_window()->number_of_frames_in_flight();
		for (int i = 0; i < concurrentFrames; ++i) {
			mViewProjBuffers.push_back(avk::context().create_buffer(
				avk::memory_usage::host_coherent, {},
				avk::uniform_buffer_meta::create_from_data(glm::mat4())
			));
		}

		mMaterialBuffer = avk::context().create_buffer(
			avk::memory_usage::host_visible, {},
			avk::storage_buffer_meta::create_from_data(gpuMaterials)
		);
		auto emptyCommand = mMaterialBuffer->fill(gpuMaterials.data(), 0);

		mImageSamplers = std::move(imageSamplers);

		vk::PhysicalDeviceMeshShaderPropertiesEXT meshShaderProps{};
		vk::PhysicalDeviceSubgroupProperties subgroupProps;
		vk::PhysicalDeviceProperties2 phProps2{};
		phProps2.pNext = &meshShaderProps;
		meshShaderProps.pNext = &subgroupProps;
		avk::context().physical_device().getProperties2(&phProps2);
		LOG_INFO(std::format("Max. preferred task threads is {}, mesh threads is {}, subgroup size is {}.",
			meshShaderProps.maxPreferredTaskWorkGroupInvocations,
			meshShaderProps.maxPreferredMeshWorkGroupInvocations,
			subgroupProps.subgroupSize));
		LOG_INFO(std::format("This device supports the following subgroup operations: {}", vk::to_string(subgroupProps.supportedOperations)));
		LOG_INFO(std::format("This device supports subgroup operations in the following stages: {}", vk::to_string(subgroupProps.supportedStages)));

		// Create our rasterization graphics pipeline with the required configuration:
		auto createGraphicsMeshPipeline = [this](auto taskShader, auto meshShader, uint32_t taskWorkGroups, uint32_t meshWorkGroups) {
			// Before creating a pipeline, let's query the VK_EXT_mesh_shader-specific device properties:
			// Also, just out of curiosity, query the subgroup properties too:
			
			return avk::context().create_graphics_pipeline_for(
				// Specify which shaders the pipeline consists of:
				avk::task_shader(taskShader)
					.set_specialization_constant(0, taskWorkGroups),
				avk::mesh_shader(meshShader)
					.set_specialization_constant(0, taskWorkGroups)
					.set_specialization_constant(1, meshWorkGroups),
				avk::fragment_shader("shaders/diffuse_shading_fixed_lightsource.frag"),
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
				avk::push_constant_binding_data{ avk::shader_type::all, 0, sizeof(push_constants) },
				avk::descriptor_binding(0, 0, avk::as_combined_image_samplers(mImageSamplers, avk::layout::shader_read_only_optimal)),
				avk::descriptor_binding(0, 1, mViewProjBuffers[0]),
				avk::descriptor_binding(1, 0, mMaterialBuffer),
				// texel buffers
				avk::descriptor_binding(3, 0, avk::as_uniform_texel_buffer_views(mPositionBuffers)),
				avk::descriptor_binding(3, 2, avk::as_uniform_texel_buffer_views(mNormalBuffers)),
				avk::descriptor_binding(3, 3, avk::as_uniform_texel_buffer_views(mTexCoordsBuffers)),
				avk::descriptor_binding(4, 0, mMeshletsBuffer)
			);
		};
		
		mPipelineEXT = createGraphicsMeshPipeline(
			"shaders/meshlet.task", "shaders/meshlet.mesh", 
			meshShaderProps.maxPreferredTaskWorkGroupInvocations, 
			meshShaderProps.maxPreferredMeshWorkGroupInvocations
		);
		// set up updater
		// we want to use an updater, so create one:
		
		mUpdater.emplace();
		mUpdater->on(avk::shader_files_changed_event(mPipelineEXT.as_reference())).update(mPipelineEXT);

		if (avk::context().supports_nv_mesh_shader(avk::context().physical_device())) {
			vk::PhysicalDeviceMeshShaderPropertiesNV meshShaderPropsNv{};
			phProps2.pNext = &meshShaderPropsNv;
			avk::context().physical_device().getProperties2(&phProps2);

			mPipelineNV = createGraphicsMeshPipeline(
				"shaders/meshlet.nv.task", "shaders/meshlet.nv.mesh",
				meshShaderPropsNv.maxTaskWorkGroupInvocations,
				meshShaderPropsNv.maxMeshWorkGroupInvocations
			);

			mUpdater->on(avk::shader_files_changed_event(mPipelineNV.as_reference())).update(mPipelineNV);

			mUseNvPipeline = false;
		}

		mUpdater->on(avk::swapchain_resized_event(avk::context().main_window())).invoke([this]() {
			this->mQuakeCam.set_aspect_ratio(avk::context().main_window()->aspect_ratio());
		});

		// Add the camera to the composition (and let it handle the updates)
		mQuakeCam.set_translation({ 0.0f, 0.0f, 0.0f });
		mQuakeCam.set_perspective_projection(glm::radians(60.0f), avk::context().main_window()->aspect_ratio(), 0.3f, 1000.0f);
		avk::current_composition()->add_element(mQuakeCam);

		auto imguiManager = avk::current_composition()->element_by_type<avk::imgui_manager>();
		if (nullptr != imguiManager) {
			imguiManager->add_callback([this]() {
				ImGui::Begin("Info & Settings");
				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
				ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), "[F1]: Toggle input-mode");
				ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), " (UI vs. scene navigation)");

				ImGui::Separator();
				if (mUseNvPipeline.has_value()) {
					int choice = mUseNvPipeline.value() ? 1 : 0;
					ImGui::Combo("Pipeline", &choice, "VK_EXT_mesh_shader\0VK_NV_mesh_shader\0");
					mUseNvPipeline = (choice == 1);
					ImGui::Separator();
				}

				// Select the range of meshlets to be rendered:
				ImGui::Checkbox("Highlight meshlets", &mHighlightMeshlets);
				ImGui::Text("Select meshlets to be rendered:");
				ImGui::DragIntRange2("Visible range", &mShowMeshletsFrom, &mShowMeshletsTo, 1, 0, static_cast<int>(mNumMeshlets));

				ImGui::End();
			});
		}
	}

	void update() override
	{
		if (avk::input().key_pressed(avk::key_code::c)) {
			// Center the cursor:
			auto resolution = avk::context().main_window()->resolution();
			avk::context().main_window()->set_cursor_pos({ resolution[0] / 2.0, resolution[1] / 2.0 });
		}
		if (avk::input().key_pressed(avk::key_code::escape)) {
			// Stop the current composition:
			avk::current_composition()->stop();
		}

		if (avk::input().key_pressed(avk::key_code::f1)) {
			auto imguiManager = avk::current_composition()->element_by_type<avk::imgui_manager>();
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

	void render() override
	{
		auto mainWnd = avk::context().main_window();
		auto ifi = mainWnd->current_in_flight_index();

		auto viewProjMat = mQuakeCam.projection_matrix() * mQuakeCam.view_matrix();
		auto emptyCmd = mViewProjBuffers[ifi]->fill(glm::value_ptr(viewProjMat), 0);
		
		// Get a command pool to allocate command buffers from:
		auto& commandPool = avk::context().get_command_pool_for_single_use_command_buffers(*mQueue);

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();
		
		// Create a command buffer and render into the *current* swap chain image:
		auto cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		auto& pipeline = mUseNvPipeline.value_or(false) ? mPipelineNV : mPipelineEXT;
		avk::context().record({
				avk::command::render_pass(pipeline->renderpass_reference(), avk::context().main_window()->current_backbuffer_reference(), {
					avk::command::bind_pipeline(pipeline.as_reference()),
					avk::command::bind_descriptors(pipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
						avk::descriptor_binding(0, 0, avk::as_combined_image_samplers(mImageSamplers, avk::layout::shader_read_only_optimal)),
						avk::descriptor_binding(0, 1, mViewProjBuffers[ifi]),
						avk::descriptor_binding(1, 0, mMaterialBuffer),
						avk::descriptor_binding(3, 0, avk::as_uniform_texel_buffer_views(mPositionBuffers)),
						avk::descriptor_binding(3, 2, avk::as_uniform_texel_buffer_views(mNormalBuffers)),
						avk::descriptor_binding(3, 3, avk::as_uniform_texel_buffer_views(mTexCoordsBuffers)),
						avk::descriptor_binding(4, 0, mMeshletsBuffer)
					})),

					avk::command::push_constants(pipeline->layout(), push_constants{
						mHighlightMeshlets,
						static_cast<int32_t>(mShowMeshletsFrom),
						static_cast<int32_t>(mShowMeshletsTo)
					}),

					// Draw all the meshlets with just one single draw call:
					avk::command::custom_commands([&,this](avk::command_buffer_t& cb) {
						if (mUseNvPipeline.value_or(false)) {
							cb.handle().drawMeshTasksNV(avk::div_ceil(mNumMeshlets, 32), 0);
						}
						else {
							cb.handle().drawMeshTasksEXT(avk::div_ceil(mNumMeshlets, 32), 1, 1);
						}
					})
				})
			})
			.into_command_buffer(cmdBfr)
			.then_submit_to(*mQueue)
			// Do not start to render before the image has become available:
			.waiting_for(imageAvailableSemaphore >> avk::stage::color_attachment_output)
			.submit();
					
		mainWnd->handle_lifetime(std::move(cmdBfr));
	}

private: // v== Member variables ==v

	avk::queue* mQueue;
	avk::descriptor_cache mDescriptorCache;

	std::vector<avk::buffer> mViewProjBuffers;
	avk::buffer mMaterialBuffer;
	avk::buffer mMeshletsBuffer;
	std::vector<avk::image_sampler> mImageSamplers;

	std::vector<data_for_draw_call> mDrawCalls;
	avk::graphics_pipeline mPipelineEXT;
	avk::graphics_pipeline mPipelineNV;
	avk::quake_camera mQuakeCam;
	uint32_t mNumMeshlets;

	std::vector<avk::buffer_view> mPositionBuffers;
	std::vector<avk::buffer_view> mTexCoordsBuffers;
	std::vector<avk::buffer_view> mNormalBuffers;

	bool mHighlightMeshlets = true;
	int  mShowMeshletsFrom  = 0;
	int  mShowMeshletsTo    = 0;
	std::optional<bool> mUseNvPipeline = {};

}; // static_meshlets_app

int main() // <== Starting point ==
{
	int result = EXIT_FAILURE;
	try {
		// Create a window and open it
		auto mainWnd = avk::context().create_window("Static Meshlets");

		mainWnd->set_resolution({ 1920, 1080 });
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
		auto app = static_meshlets_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = avk::imgui_manager(singleQueue);

		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			avk::application_name("Auto-Vk-Toolkit Example: Static Meshlets"),
			// Gotta enable the mesh shader extension, ...
			avk::required_device_extensions(VK_EXT_MESH_SHADER_EXTENSION_NAME)
				.add_extension(VK_NV_MESH_SHADER_EXTENSION_NAME),
			// ... and enable the mesh shader features that we need:
			[](vk::PhysicalDeviceMeshShaderFeaturesEXT& meshShaderFeatures) {
				meshShaderFeatures.setMeshShader(VK_TRUE);
				meshShaderFeatures.setTaskShader(VK_TRUE);
			},
			[](vk::PhysicalDeviceVulkan12Features& features) {
				features.setUniformAndStorageBuffer8BitAccess(VK_TRUE);
				features.setStorageBuffer8BitAccess(VK_TRUE);
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
