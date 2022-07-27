#include <auto_vk_toolkit.hpp>
#include <imgui.h>
#include "../shaders/cpu_gpu_shared_config.h"

#define USE_CACHE 1

static constexpr size_t sNumVertices = 64;
static constexpr size_t sNumIndices = 378;
static constexpr uint32_t cConcurrentFrames = 3u;

class skinned_meshlets_app : public avk::invokee
{
	struct alignas(16) push_constants
	{
		VkBool32 mHighlightMeshlets;
	};

	/** Contains the necessary buffers for drawing everything */
	struct data_for_draw_call
	{
		avk::buffer mPositionsBuffer;
		avk::buffer mTexCoordsBuffer;
		avk::buffer mNormalsBuffer;
		avk::buffer mBoneIndicesBuffer;
		avk::buffer mBoneWeightsBuffer;
#if USE_REDIRECTED_GPU_DATA
		avk::buffer mMeshletDataBuffer;
#endif

		glm::mat4 mModelMatrix;

		uint32_t mMaterialIndex;
		uint32_t mModelIndex;
	};

	/** Contains the data for each draw call */
	struct loaded_data_for_draw_call
	{
		std::vector<glm::vec3> mPositions;
		std::vector<glm::vec2> mTexCoords;
		std::vector<glm::vec3> mNormals;
		std::vector<uint32_t> mIndices;
		std::vector<glm::uvec4> mBoneIndices;
		std::vector<glm::vec4> mBoneWeights;
#if USE_REDIRECTED_GPU_DATA
		std::vector<uint32_t> mMeshletData;
#endif

		glm::mat4 mModelMatrix;

		uint32_t mMaterialIndex;
		uint32_t mModelIndex;
	};

	struct additional_animated_model_data
	{
		std::vector<glm::mat4> mBoneMatricesAni;
	};

	/** Helper struct for the animations. */
	struct animated_model_data
	{
		animated_model_data() = default;
		animated_model_data(animated_model_data&&) = default;
		animated_model_data& operator=(animated_model_data&&) = default;
		~animated_model_data() = default;
		// prevent copying of the data:
		animated_model_data(const animated_model_data&) = delete;
		animated_model_data& operator=(const animated_model_data&) = delete;

		std::string mModelName;
		avk::animation_clip_data mClip;
		uint32_t mNumBoneMatrices;
		size_t mBoneMatricesBufferIndex;
		avk::animation mAnimation;

		[[nodiscard]] double start_sec() const { return mClip.mStartTicks / mClip.mTicksPerSecond; }
		[[nodiscard]] double end_sec() const { return mClip.mEndTicks / mClip.mTicksPerSecond; }
		[[nodiscard]] double duration_sec() const { return end_sec() - start_sec(); }
		[[nodiscard]] double duration_ticks() const { return mClip.mEndTicks - mClip.mStartTicks; }
	};

	/** The meshlet we upload to the gpu with its additional data. */
	struct alignas(16) meshlet
	{
		glm::mat4 mTransformationMatrix;
		uint32_t mMaterialIndex;
		uint32_t mTexelBufferIndex;
		uint32_t mModelIndex;

#if !USE_REDIRECTED_GPU_DATA
		avk::meshlet_gpu_data<sNumVertices, sNumIndices> mGeometry;
#else
		avk::meshlet_redirected_gpu_data mGeometry;
#endif
	};

public: // v== avk::invokee overrides which will be invoked by the framework ==v
	skinned_meshlets_app(avk::queue& aQueue)
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

#if USE_REDIRECTED_GPU_DATA
			drawCall.mMeshletDataBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_data(drawCallData.mMeshletData),
				avk::storage_buffer_meta::create_from_data(drawCallData.mMeshletData),
				avk::uniform_texel_buffer_meta::create_from_data(drawCallData.mMeshletData).describe_only_member(drawCallData.mMeshletData[0])
			);
#endif

			drawCall.mBoneIndicesBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_data(drawCallData.mBoneIndices),
				avk::storage_buffer_meta::create_from_data(drawCallData.mBoneIndices),
				avk::uniform_texel_buffer_meta::create_from_data(drawCallData.mBoneIndices).describe_only_member(drawCallData.mBoneIndices[0]) // just take the uvec4 as it is 
			);

			drawCall.mBoneWeightsBuffer = avk::context().create_buffer(avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_data(drawCallData.mBoneWeights),
				avk::storage_buffer_meta::create_from_data(drawCallData.mBoneWeights),
				avk::uniform_texel_buffer_meta::create_from_data(drawCallData.mBoneWeights).describe_only_member(drawCallData.mBoneWeights[0]) // just take the vec4 as it is 
			);

			avk::context().record_and_submit_with_fence({
				drawCall.mPositionsBuffer->fill(drawCallData.mPositions.data(), 0),
				drawCall.mNormalsBuffer->fill(drawCallData.mNormals.data(), 0),
				drawCall.mTexCoordsBuffer->fill(drawCallData.mTexCoords.data(), 0),
				drawCall.mBoneIndicesBuffer->fill(drawCallData.mBoneIndices.data(), 0),
				drawCall.mBoneWeightsBuffer->fill(drawCallData.mBoneWeights.data(), 0)
#if USE_REDIRECTED_GPU_DATA
				, drawCall.mMeshletDataBuffer->fill(drawCallData.mMeshletData.data(), 0)
#endif
				}, *mQueue)->wait_until_signalled();

			// add them to the texel buffers
			mPositionBuffers.push_back(avk::context().create_buffer_view(drawCall.mPositionsBuffer));
			mNormalBuffers.push_back(avk::context().create_buffer_view(drawCall.mNormalsBuffer));
			mTexCoordsBuffers.push_back(avk::context().create_buffer_view(drawCall.mTexCoordsBuffer));
#if USE_REDIRECTED_GPU_DATA
			mMeshletDataBuffers.push_back(avk::context().create_buffer_view(drawCall.mMeshletDataBuffer));
#endif
			mBoneIndicesBuffers.push_back(avk::context().create_buffer_view(drawCall.mBoneIndicesBuffer));
			mBoneWeightsBuffers.push_back(avk::context().create_buffer_view(drawCall.mBoneWeightsBuffer));
		}
	}

	void initialize() override
	{
		// use helper functions to create ImGui elements
		auto surfaceCap = avk::context().physical_device().getSurfaceCapabilitiesKHR(avk::context().main_window()->surface());

		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = avk::context().create_descriptor_cache();

		glm::mat4 globalTransform = glm::rotate(glm::radians(180.f), glm::vec3(0.f, 1.f, 0.f)) * glm::scale(glm::vec3(1.f));
		std::vector<avk::model> loadedModels;
		// Load a model from file:
		auto model = avk::model_t::load_from_file("assets/crab.fbx", aiProcess_Triangulate);

		loadedModels.push_back(std::move(model));

		std::vector<avk::material_config> allMatConfigs; // <-- Gather the material config from all models to be loaded
		std::vector<loaded_data_for_draw_call> dataForDrawCall;
		std::vector<meshlet> meshletsGeometry;
		std::vector<animated_model_data> animatedModels;

		// Crab-specific animation config: (Needs to be adapted for other models)
		const uint32_t cAnimationIndex = 0;
		const uint32_t cStartTimeTicks = 0;
		const uint32_t cEndTimeTicks   = 58;
		const uint32_t cTicksPerSecond = 34;

		// Generate the meshlets for each loaded model.
		for (size_t i = 0; i < loadedModels.size(); ++i) {
			auto curModel = std::move(loadedModels[i]);

			// load the animation
			auto curClip = curModel->load_animation_clip(cAnimationIndex, cStartTimeTicks, cEndTimeTicks);
			curClip.mTicksPerSecond = cTicksPerSecond;
			auto& curEntry = animatedModels.emplace_back();
			curEntry.mModelName = curModel->path();
			curEntry.mClip = curClip;

			// get all the meshlet indices of the model
			const auto meshIndicesInOrder = curModel->select_all_meshes();

			curEntry.mNumBoneMatrices = curModel->num_bone_matrices(meshIndicesInOrder);

			// Store offset into the vector of buffers that store the bone matrices
			curEntry.mBoneMatricesBufferIndex = i;

			auto distinctMaterials = curModel->distinct_material_configs();
			const auto matOffset = allMatConfigs.size();
			// add all the materials of the model
			for (auto& pair : distinctMaterials) {
				allMatConfigs.push_back(pair.first);
			}

			// prepare the animation for the current entry
			curEntry.mAnimation = curModel->prepare_animation(curEntry.mClip.mAnimationIndex, meshIndicesInOrder);

			// Generate meshlets for each submesh of the current loaded model. Load all it's data into the drawcall for later use.
			for (size_t mpos = 0; mpos < meshIndicesInOrder.size(); mpos++) {
				auto meshIndex = meshIndicesInOrder[mpos];
				std::string meshname = curModel->name_of_mesh(mpos);

				auto texelBufferIndex = dataForDrawCall.size();
				auto& drawCallData = dataForDrawCall.emplace_back();

				drawCallData.mMaterialIndex = static_cast<int32_t>(matOffset);
				drawCallData.mModelMatrix = globalTransform;
				drawCallData.mModelIndex = static_cast<uint32_t>(curEntry.mBoneMatricesBufferIndex);
				// Find and assign the correct material (in the ~"global" allMatConfigs vector!)
				for (auto pair : distinctMaterials) {
					if (std::end(pair.second) != std::find(std::begin(pair.second), std::end(pair.second), meshIndex)) {
						break;
					}

					drawCallData.mMaterialIndex++;
				}

				auto selection = avk::make_model_references_and_mesh_indices_selection(curModel, meshIndex);
				std::vector<avk::mesh_index_t> meshIndices;
				meshIndices.push_back(meshIndex);
				// Build meshlets:
				std::tie(drawCallData.mPositions, drawCallData.mIndices) = avk::get_vertices_and_indices(selection);
				drawCallData.mNormals = avk::get_normals(selection);
				drawCallData.mTexCoords = avk::get_2d_texture_coordinates(selection, 0);
				// Get bone indices and weights too
				drawCallData.mBoneIndices = avk::get_bone_indices_for_single_target_buffer(selection, meshIndicesInOrder);
				drawCallData.mBoneWeights = avk::get_bone_weights(selection);

				// create selection for the meshlets
				auto meshletSelection = avk::make_models_and_mesh_indices_selection(curModel, meshIndex);

				auto cpuMeshlets = avk::divide_into_meshlets(meshletSelection);
#if !USE_REDIRECTED_GPU_DATA
#if USE_CACHE
				avk::serializer serializer("direct_meshlets-" + meshname + "-" + std::to_string(mpos) + ".cache");
				auto [gpuMeshlets, _] = avk::convert_for_gpu_usage_cached<avk::meshlet_gpu_data<sNumVertices, sNumIndices>>(serializer, cpuMeshlets);
#else
				auto [gpuMeshlets, _] = avk::convert_for_gpu_usage<avk::meshlet_gpu_data<sNumVertices, sNumIndices>, sNumVertices, sNumIndices>(cpuMeshlets);
#endif
#else
#if USE_CACHE
				avk::serializer serializer("indirect_meshlets-" + meshname + "-" + std::to_string(mpos) + ".cache");
				auto [gpuMeshlets, generatedMeshletData] = avk::convert_for_gpu_usage_cached<avk::meshlet_redirected_gpu_data, sNumVertices, sNumIndices>(serializer, cpuMeshlets);
#else
				auto [gpuMeshlets, generatedMeshletData] = avk::convert_for_gpu_usage<avk::meshlet_redirected_gpu_data, sNumVertices, sNumIndices>(cpuMeshlets);
#endif
				drawCallData.mMeshletData = std::move(generatedMeshletData.value());
#endif

				// fill our own meshlets with the loaded/generated data
				for (size_t mshltidx = 0; mshltidx < gpuMeshlets.size(); ++mshltidx) {
					auto& genMeshlet = gpuMeshlets[mshltidx];

					auto& ml = meshletsGeometry.emplace_back(meshlet{});

#pragma region start to assemble meshlet struct
					ml.mTransformationMatrix = drawCallData.mModelMatrix;
					ml.mMaterialIndex = drawCallData.mMaterialIndex;
					ml.mTexelBufferIndex = static_cast<uint32_t>(texelBufferIndex);
					ml.mModelIndex = static_cast<uint32_t>(curEntry.mBoneMatricesBufferIndex);

					ml.mGeometry = genMeshlet;
#pragma endregion 
				}
			}
		} // for (size_t i = 0; i < loadedModels.size(); ++i)

		// create buffers for animation data
		for (size_t i = 0; i < loadedModels.size(); ++i) {
			auto& animModel = mAnimatedModels.emplace_back(std::move(animatedModels[i]), additional_animated_model_data{});

			// buffers for the animated bone matrices, will be populated before rendering
			std::get<additional_animated_model_data>(animModel).mBoneMatricesAni.resize(std::get<animated_model_data>(animModel).mNumBoneMatrices);
			for (size_t cfi = 0; cfi < cConcurrentFrames; ++cfi) {
				mBoneMatricesBuffersAni[cfi].push_back(avk::context().create_buffer(
					avk::memory_usage::host_coherent, {},
					avk::storage_buffer_meta::create_from_data(std::get<additional_animated_model_data>(animModel).mBoneMatricesAni)
				));
			}
		}
		// create all the buffers for our drawcall data
		add_draw_calls(dataForDrawCall, mDrawCalls);

		// Put the meshlets that we have gathered into a buffer:
		mMeshletsBuffer = avk::context().create_buffer(
			avk::memory_usage::device, {},
			avk::storage_buffer_meta::create_from_data(meshletsGeometry)
		);
		mNumMeshletWorkgroups = meshletsGeometry.size();

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
			}, * mQueue)->wait_until_signalled();

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

		auto swapChainFormat = avk::context().main_window()->swap_chain_image_format();
		// Create our rasterization graphics pipeline with the required configuration:
		mPipeline = avk::context().create_graphics_pipeline_for(
			// Specify which shaders the pipeline consists of:
			avk::task_shader("shaders/meshlet.task"),
			avk::mesh_shader("shaders/meshlet.mesh"),
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
			avk::push_constant_binding_data{ avk::shader_type::fragment, 0, sizeof(push_constants) },
			avk::descriptor_binding(0, 0, avk::as_combined_image_samplers(mImageSamplers, avk::layout::shader_read_only_optimal)),
			avk::descriptor_binding(0, 1, mViewProjBuffers[0]),
			avk::descriptor_binding(1, 0, mMaterialBuffer),
			avk::descriptor_binding(2, 0, mBoneMatricesBuffersAni[0]),
			// texel buffers
			avk::descriptor_binding(3, 0, avk::as_uniform_texel_buffer_views(mPositionBuffers)),
			avk::descriptor_binding(3, 2, avk::as_uniform_texel_buffer_views(mNormalBuffers)),
			avk::descriptor_binding(3, 3, avk::as_uniform_texel_buffer_views(mTexCoordsBuffers)),
#if USE_REDIRECTED_GPU_DATA
			avk::descriptor_binding(3, 4, avk::as_uniform_texel_buffer_views(mMeshletDataBuffers)),
#endif
			avk::descriptor_binding(3, 5, avk::as_uniform_texel_buffer_views(mBoneIndicesBuffers)),
			avk::descriptor_binding(3, 6, avk::as_uniform_texel_buffer_views(mBoneWeightsBuffers)),
			avk::descriptor_binding(4, 0, mMeshletsBuffer)
		);

		// set up updater
		// we want to use an updater, so create one:

		mUpdater.emplace();
		mUpdater->on(avk::shader_files_changed_event(mPipeline.as_reference())).update(mPipeline);

		mUpdater->on(avk::swapchain_resized_event(avk::context().main_window())).invoke([this]() {
			this->mQuakeCam.set_aspect_ratio(avk::context().main_window()->aspect_ratio());
		});

		// Add the camera to the composition (and let it handle the updates)
		mQuakeCam.set_translation({ 0.0f, -1.0f, 8.0f });
		mQuakeCam.set_perspective_projection(glm::radians(60.0f), avk::context().main_window()->aspect_ratio(), 0.3f, 1000.0f);
		//mQuakeCam.set_orthographic_projection(-5, 5, -5, 5, 0.5, 100);
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

				ImGui::Checkbox("Highlight Meshlets", &mHighlightMeshlets);

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
			mQuakeCam.look_at(glm::vec3{ 0.0f, 0.0f, 0.0f });
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

		// Animate all the meshes
		for (auto& model : mAnimatedModels) {
			auto& animation = std::get<animated_model_data>(model).mAnimation;
			auto& clip = std::get<animated_model_data>(model).mClip;
			const auto doubleTime = fmod(avk::time().absolute_time_dp(), std::get<animated_model_data>(model).duration_sec() * 2);
			auto time = glm::lerp(std::get<animated_model_data>(model).start_sec(), std::get<animated_model_data>(model).end_sec(), (doubleTime > std::get<animated_model_data>(model).duration_sec() ? doubleTime - std::get<animated_model_data>(model).duration_sec() : doubleTime) / std::get<animated_model_data>(model).duration_sec());
			auto targetMemory = std::get<additional_animated_model_data>(model).mBoneMatricesAni.data();

			// Use lambda option 1 that takes as parameters: mesh_bone_info, inverse mesh root matrix, global node/bone transform w.r.t. the animation, inverse bind-pose matrix
			animation.animate(clip, time, [this, &animation, targetMemory](avk::mesh_bone_info aInfo, const glm::mat4& aInverseMeshRootMatrix, const glm::mat4& aTransformMatrix, const glm::mat4& aInverseBindPoseMatrix, const glm::mat4& aLocalTransformMatrix, size_t aAnimatedNodeIndex, size_t aBoneMeshTargetIndex, double aAnimationTimeInTicks) {
				// Construction of the bone matrix for this node:
				//   1. Bring vertex into bone space
				//   2. Apply transformaton in bone space => MODEL SPACE
				targetMemory[aInfo.mGlobalBoneIndexOffset + aInfo.mMeshLocalBoneIndex] = aTransformMatrix * aInverseBindPoseMatrix;
			});
		}

		auto viewProjMat = mQuakeCam.projection_matrix() * mQuakeCam.view_matrix();
		auto emptyCmd = mViewProjBuffers[ifi]->fill(glm::value_ptr(viewProjMat), 0);
		
		// Get a command pool to allocate command buffers from:
		auto& commandPool = avk::context().get_command_pool_for_single_use_command_buffers(*mQueue);

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();
		
		// Create a command buffer and render into the *current* swap chain image:
		auto cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		
		avk::context().record(avk::command::gather(
				// Upload the updated bone matrices into the buffer for the current frame (considering that we have cConcurrentFrames-many concurrent frames):
				avk::command::one_for_each(mAnimatedModels, [this, ifi](const std::tuple<animated_model_data, additional_animated_model_data>& tpl){
					return mBoneMatricesBuffersAni[ifi][std::get<animated_model_data>(tpl).mBoneMatricesBufferIndex]->fill(std::get<additional_animated_model_data>(tpl).mBoneMatricesAni.data(), 0);
				}),

				avk::command::render_pass(mPipeline->renderpass_reference(), avk::context().main_window()->current_backbuffer_reference(), {
					avk::command::bind_pipeline(mPipeline.as_reference()),
					avk::command::bind_descriptors(mPipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
						avk::descriptor_binding(0, 0, avk::as_combined_image_samplers(mImageSamplers, avk::layout::shader_read_only_optimal)),
						avk::descriptor_binding(0, 1, mViewProjBuffers[ifi]),
						avk::descriptor_binding(1, 0, mMaterialBuffer),
						avk::descriptor_binding(2, 0, mBoneMatricesBuffersAni[ifi]),
						avk::descriptor_binding(3, 0, avk::as_uniform_texel_buffer_views(mPositionBuffers)),
						avk::descriptor_binding(3, 2, avk::as_uniform_texel_buffer_views(mNormalBuffers)),
						avk::descriptor_binding(3, 3, avk::as_uniform_texel_buffer_views(mTexCoordsBuffers)),
#if USE_REDIRECTED_GPU_DATA
						avk::descriptor_binding(3, 4, avk::as_uniform_texel_buffer_views(mMeshletDataBuffers)),
#endif
						avk::descriptor_binding(3, 5, avk::as_uniform_texel_buffer_views(mBoneIndicesBuffers)),
						avk::descriptor_binding(3, 6, avk::as_uniform_texel_buffer_views(mBoneWeightsBuffers)),
						avk::descriptor_binding(4, 0, mMeshletsBuffer)
					})),

					avk::command::push_constants(mPipeline->layout(), push_constants{ mHighlightMeshlets }),

					// Draw all the meshlets with just one single draw call:
					avk::command::custom_commands([&,this](avk::command_buffer_t& cb) { 
						cb.handle().drawMeshTasksNV(mNumMeshletWorkgroups, 0);
					})
				})
			))
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

	std::vector<std::tuple<animated_model_data, additional_animated_model_data>> mAnimatedModels;

	std::vector<avk::buffer> mViewProjBuffers;
	avk::buffer mMaterialBuffer;
	avk::buffer mMeshletsBuffer;
	std::array<std::vector<avk::buffer>, cConcurrentFrames> mBoneMatricesBuffersAni;
	std::vector<avk::image_sampler> mImageSamplers;

	std::vector<data_for_draw_call> mDrawCalls;
	avk::graphics_pipeline mPipeline;
	avk::quake_camera mQuakeCam;
	size_t mNumMeshletWorkgroups;

	std::vector<avk::buffer_view> mPositionBuffers;
	std::vector<avk::buffer_view> mTexCoordsBuffers;
	std::vector<avk::buffer_view> mNormalBuffers;
	std::vector<avk::buffer_view> mBoneWeightsBuffers;
	std::vector<avk::buffer_view> mBoneIndicesBuffers;
#if USE_REDIRECTED_GPU_DATA
	std::vector<avk::buffer_view> mMeshletDataBuffers;
#endif

	bool mHighlightMeshlets;

}; // skinned_meshlets_app

int main() // <== Starting point ==
{
	int result = EXIT_FAILURE;
	try {
		// Create a window and open it
		auto mainWnd = avk::context().create_window("Skinned Meshlets");

		mainWnd->set_resolution({ 1920, 1080 });
		mainWnd->enable_resizing(true);
		mainWnd->set_additional_back_buffer_attachments({
			avk::attachment::declare(vk::Format::eD32Sfloat, avk::on_load::clear.from_previous_layout(avk::layout::undefined), avk::usage::depth_stencil, avk::on_store::dont_care)
		});
		mainWnd->set_presentaton_mode(avk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(cConcurrentFrames);
		mainWnd->open();

		auto& singleQueue = avk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->set_queue_family_ownership(singleQueue.family_index());
		mainWnd->set_present_queue(singleQueue);

		// Create an instance of our main avk::element which contains all the functionality:
		auto app = skinned_meshlets_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = avk::imgui_manager(singleQueue);

		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			avk::application_name("Auto-Vk-Toolkit Example: Skinned Meshlets"),
			avk::required_device_extensions(VK_NV_MESH_SHADER_EXTENSION_NAME)
			.add_extension(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME),
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
