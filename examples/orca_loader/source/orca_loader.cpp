#include <gvk.hpp>
#include <imgui.h>
// Use ImGui::FileBrowser from here: https://github.com/AirGuanZ/imgui-filebrowser
#include "imfilebrowser.h"
#include <glm/gtx/euler_angles.hpp>

#define USE_SERIALIZER

class orca_loader_app : public gvk::invokee
{
	struct data_for_draw_call
	{
		avk::buffer mPositionsBuffer;
		avk::buffer mTexCoordsBuffer;
		avk::buffer mNormalsBuffer;
		avk::buffer mIndexBuffer;
		int mMaterialIndex;
		glm::mat4 mModelMatrix;
	};

	struct transformation_matrices {
		glm::mat4 mModelMatrix;
		int mMaterialIndex;
	};

public: // v== avk::invokee overrides which will be invoked by the framework ==v
	orca_loader_app(avk::queue& aQueue)
		: mQueue{ &aQueue }
		, mFileBrowser{ ImGuiFileBrowserFlags_EnterNewFilename }
	{}

	// Loads an ORCA scene from file by performing the following steps:
	//  - Destroy the resources representing the currently loaded scene in n frames
	//    (where n is the number of frames in flight). The resources to be destroyed are:
	//     - mDrawCalls
	//     - mMaterialBuffer
	//     - mImageSamplers
	//  - Load ORCA scene from file, creating the resources anew:
	//     - mDrawCalls
	//     - mMaterialBuffer
	//     - mImageSamplers
	void load_orca_scene(const std::string& aPathToOrcaScene)
	{
		// Clean up the current resources, before creating new ones:
		mOldDrawCalls = std::move(mDrawCalls);
		mOldImageSamplers =  std::move(mImageSamplers);
		mOldMaterialBuffer = std::move(mMaterialBuffer);
		mOldPipeline = std::move(mPipeline);
		// In #number_of_frames_in_flight() into the future, it will be safe to delete the old resources in render()!
		// In update() it is not because the fence-wait that ensures that the resources are not used anymore, happens between update() and render().
		mDestroyOldResourcesInFrame = gvk::context().main_window()->current_frame() + gvk::context().main_window()->number_of_frames_in_flight(); 
		
		float start = gvk::context().get_time();
		float startPart = start;
		float endPart = 0.0f;
		std::vector<std::tuple<std::string, float>> times;

		// Load an ORCA scene from file:
		auto orca = gvk::orca_scene_t::load_from_file(aPathToOrcaScene);
		// Get all the different materials from the whole scene:
		auto distinctMaterialsOrca = orca->distinct_material_configs_for_all_models();

		endPart = gvk::context().get_time();
		times.emplace_back(std::make_tuple("load orca file", endPart - startPart));
		startPart = gvk::context().get_time();

		// The following loop gathers all the vertex and index data PER MATERIAL and constructs the buffers and materials.
		// Later, we'll use ONE draw call PER MATERIAL to draw the whole scene.
		std::vector<gvk::material_config> allMatConfigs;
		mDrawCalls.clear();
		for (const auto& pair : distinctMaterialsOrca) {
			allMatConfigs.push_back(pair.first);
			const int matIndex = static_cast<int>(allMatConfigs.size()) - 1;

			// The data in distinctMaterialsOrca encompasses all of the ORCA scene's models.
			for (const auto& indices : pair.second) {
				// However, we have to pay attention to the specific model's scene-properties,...
				auto& modelData = orca->model_at_index(indices.mModelIndex);
				// ... specifically, to its instances:
				
				// Get a buffer containing all positions, and one containing all indices for all submeshes with this material
				auto [positionsBuffer, indicesBuffer] = gvk::create_vertex_and_index_buffers(
					{ gvk::make_models_and_meshes_selection(modelData.mLoadedModel, indices.mMeshIndices) }, {},
					avk::sync::wait_idle()
				);
				positionsBuffer.enable_shared_ownership(); // Enable multiple owners of this buffer, because there might be multiple model-instances and hence, multiple draw calls that want to use this buffer.
				indicesBuffer.enable_shared_ownership(); // Enable multiple owners of this buffer, because there might be multiple model-instances and hence, multiple draw calls that want to use this buffer.

				// Get a buffer containing all texture coordinates for all submeshes with this material
				auto texCoordsBuffer = gvk::create_2d_texture_coordinates_flipped_buffer(
					{ gvk::make_models_and_meshes_selection(modelData.mLoadedModel, indices.mMeshIndices) }, 0,
					avk::sync::wait_idle()
				);
				texCoordsBuffer.enable_shared_ownership(); // Enable multiple owners of this buffer, because there might be multiple model-instances and hence, multiple draw calls that want to use this buffer.

				// Get a buffer containing all normals for all submeshes with this material
				auto normalsBuffer = gvk::create_normals_buffer(
					{ gvk::make_models_and_meshes_selection(modelData.mLoadedModel, indices.mMeshIndices) }, 
					avk::sync::wait_idle()
				);
				normalsBuffer.enable_shared_ownership(); // Enable multiple owners of this buffer, because there might be multiple model-instances and hence, multiple draw calls that want to use this buffer.

				for (size_t i = 0; i < modelData.mInstances.size(); ++i) {
					auto& newElement = mDrawCalls.emplace_back();
					newElement.mMaterialIndex = matIndex;
					newElement.mModelMatrix = gvk::matrix_from_transforms(modelData.mInstances[i].mTranslation, glm::quat(modelData.mInstances[i].mRotation), modelData.mInstances[i].mScaling);
					newElement.mPositionsBuffer = positionsBuffer;
					newElement.mIndexBuffer = indicesBuffer;
					newElement.mTexCoordsBuffer = texCoordsBuffer;
					newElement.mNormalsBuffer = normalsBuffer;
				}
			}
		}

		endPart = gvk::context().get_time();
		times.emplace_back(std::make_tuple("create materials config", endPart - startPart));
		startPart = gvk::context().get_time();

		// Convert the materials that were gathered above into a GPU-compatible format, and upload into a GPU storage buffer:
		auto [gpuMaterials, imageSamplers] = gvk::convert_for_gpu_usage(
			allMatConfigs, false, false,
			avk::image_usage::general_texture,
			avk::filter_mode::anisotropic_16x,
			avk::border_handling_mode::repeat,
			avk::sync::wait_idle()
		);

		endPart = gvk::context().get_time();
		times.emplace_back(std::make_tuple("convert_for_gpu_usage", endPart - startPart));
		startPart = gvk::context().get_time();

		for (auto& t : times) {
			LOG_INFO(fmt::format("{} took {}", std::get<0>(t), std::get<1>(t)));
		}

		float end = gvk::context().get_time();
		float diff = end - start;
		LOG_INFO(fmt::format("serialization/deserialization took total {}", diff));

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
			avk::vertex_shader("shaders/transform_and_pass_pos_nrm_uv.vert"),
			avk::fragment_shader("shaders/diffuse_shading_fixed_lightsource.frag"),
			// The next 3 lines define the format and location of the vertex shader inputs:
			// (The dummy values (like glm::vec3) tell the pipeline the format of the respective input)
			avk::from_buffer_binding(0) -> stream_per_vertex<glm::vec3>() -> to_location(0), // <-- corresponds to vertex shader's inPosition
			avk::from_buffer_binding(1) -> stream_per_vertex<glm::vec2>() -> to_location(1), // <-- corresponds to vertex shader's inTexCoord
			avk::from_buffer_binding(2) -> stream_per_vertex<glm::vec3>() -> to_location(2), // <-- corresponds to vertex shader's inNormal
			// Some further settings:
			avk::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(gvk::context().main_window()->backbuffer_at_index(0)),
			// We'll render to the back buffer, which has a color attachment always, and in our case additionally a depth 
			// attachment, which has been configured when creating the window. See main() function!
			avk::attachment::declare(gvk::format_from_window_color_buffer(gvk::context().main_window()), avk::on_load::clear, avk::color(0),		avk::on_store::store),	 // But not in presentable format, because ImGui comes after
			avk::attachment::declare(gvk::format_from_window_depth_buffer(gvk::context().main_window()), avk::on_load::clear, avk::depth_stencil(), avk::on_store::dont_care),
			// The following define additional data which we'll pass to the pipeline:
			//   We'll pass two matrices to our vertex shader via push constants:
			avk::push_constant_binding_data { avk::shader_type::vertex, 0, sizeof(transformation_matrices) },
			avk::descriptor_binding(0, 5, mViewProjBuffer),
			avk::descriptor_binding(4, 4, mImageSamplers),
			avk::descriptor_binding(7, 9, mMaterialBuffer) 
		);
	}

	// Loads an ORCA scene from file or cache file by performing the following steps:
	//  - Destroy the resources representing the currently loaded scene in n frames
	//    (where n is the number of frames in flight). The resources to be destroyed are:
	//     - mDrawCalls
	//     - mMaterialBuffer
	//     - mImageSamplers
	//  - Load ORCA scene from file, creating the resources anew:
	//     - mDrawCalls
	//     - mMaterialBuffer
	//     - mImageSamplers
	void load_orca_scene_cached(const std::string& aPathToOrcaScene)
	{
		// Clean up the current resources, before creating new ones:
		mOldDrawCalls = std::move(mDrawCalls);
		mOldImageSamplers =  std::move(mImageSamplers);
		mOldMaterialBuffer = std::move(mMaterialBuffer);
		mOldPipeline = std::move(mPipeline);
		// In #number_of_frames_in_flight() into the future, it will be safe to delete the old resources in render()!
		// In update() it is not because the fence-wait that ensures that the resources are not used anymore, happens between update() and render().
		mDestroyOldResourcesInFrame = gvk::context().main_window()->current_frame() + gvk::context().main_window()->number_of_frames_in_flight(); 
		
		gvk::orca_scene orca;
		std::unordered_map<gvk::material_config, std::vector<gvk::model_and_mesh_indices>> distinctMaterialsOrca;

		const std::string cacheFilePath(aPathToOrcaScene + ".cache");
		// If a cache file exists, i.e. the scene was serialized during a previous load, initialize the serializer in deserialize mode,
		// else initialize the serializer in serialize mode to create the cache file while processing the scene.
		auto serializer = gvk::does_cache_file_exist(cacheFilePath) ?
			gvk::serializer(gvk::serializer::deserialize(cacheFilePath)) :
			gvk::serializer(gvk::serializer::serialize(cacheFilePath));

		float start = gvk::context().get_time();
		float startPart = start;
		float endPart = 0.0f;
		std::vector<std::tuple<std::string, float>> times;

		// Load orca scene for usage and serialization, loading the scene is not required if a cache file exists, i.e. mode == deserialize
		if (serializer.mode() == gvk::serializer::mode::serialize) {
			// Load an ORCA scene from file:
			orca = gvk::orca_scene_t::load_from_file(aPathToOrcaScene);
			// Get all the different materials from the whole scene:
			distinctMaterialsOrca = orca->distinct_material_configs_for_all_models();

			endPart = gvk::context().get_time();
			times.emplace_back(std::make_tuple("no cache file, loading orca file", endPart - startPart));
			startPart = gvk::context().get_time();
		}

		// Get number of distinc materials from orca scene and serialize it or retrieve the number from the serializer
		size_t numDistinctMaterials = (serializer.mode() == gvk::serializer::mode::serialize) ? distinctMaterialsOrca.size() : 0;
		serializer.archive(numDistinctMaterials);

		// The following loop gathers all the vertex and index data PER MATERIAL and constructs the buffers and materials.
		// Later, we'll use ONE draw call PER MATERIAL to draw the whole scene.
		std::vector<gvk::material_config> allMatConfigs;
		mDrawCalls.clear();
		auto materials = distinctMaterialsOrca.begin();
		for (int materialIndex = 0; materialIndex < numDistinctMaterials; ++materialIndex) {
			// meshIndices is only needed during serialization, otherwise the serializer handles everything
			// in the respective *_cached functions and meshIndices may be empty when passed to the
			// respective functions.
			size_t numMeshIndices;
			std::vector<gvk::model_and_mesh_indices> meshIndices;
			if (serializer.mode() == gvk::serializer::mode::serialize) {
				allMatConfigs.push_back(materials->first);
				meshIndices = materials->second;
				numMeshIndices = materials->second.size();
				materials = std::next(materials);
			}
			// Serialize or retrieve the number of model_and_mesh_indices for the material
			serializer.archive(numMeshIndices);

			for (int meshIndicesIndex = 0; meshIndicesIndex < numMeshIndices; ++meshIndicesIndex) {
				// Convinience function to retrieve the model data via the mesh indices from the orca scene while in serialize mode
				auto getModelData = [&]() -> gvk::model_data& { return orca->model_at_index(meshIndices[meshIndicesIndex].mModelIndex); };

				// modelAndMeshes is only needed during serialization, otherwise the following buffers are filled by the
				// serializer from the cache file in the repsective *_cached functions and modelAndMeshes may be empty.
				std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<size_t>>> modelAndMeshes;
				if (serializer.mode() == gvk::serializer::mode::serialize) {
					modelAndMeshes = gvk::make_models_and_meshes_selection(getModelData().mLoadedModel, meshIndices[meshIndicesIndex].mMeshIndices);
				}

				// Get a buffer containing all positions, and one containing all indices for all submeshes with this material
				auto [positionsBuffer, indicesBuffer] = gvk::create_vertex_and_index_buffers_cached(
					serializer, modelAndMeshes, {}, avk::sync::wait_idle()
				);
				positionsBuffer.enable_shared_ownership(); // Enable multiple owners of this buffer, because there might be multiple model-instances and hence, multiple draw calls that want to use this buffer.
				indicesBuffer.enable_shared_ownership(); // Enable multiple owners of this buffer, because there might be multiple model-instances and hence, multiple draw calls that want to use this buffer.

				// Get a buffer containing all texture coordinates for all submeshes with this material
				auto texCoordsBuffer = gvk::create_2d_texture_coordinates_flipped_buffer_cached(
					serializer, modelAndMeshes, 0, avk::sync::wait_idle()
				);
				texCoordsBuffer.enable_shared_ownership(); // Enable multiple owners of this buffer, because there might be multiple model-instances and hence, multiple draw calls that want to use this buffer.

				// Get a buffer containing all normals for all submeshes with this material
				auto normalsBuffer = gvk::create_normals_buffer_cached(
					serializer, modelAndMeshes, avk::sync::wait_idle()
				);
				normalsBuffer.enable_shared_ownership(); // Enable multiple owners of this buffer, because there might be multiple model-instances and hence, multiple draw calls that want to use this buffer.

				// Get the number of instances from the model and serialize it or retrieve it from the serializer
				size_t numInstances = (serializer.mode() == gvk::serializer::mode::serialize) ? getModelData().mInstances.size() : 0;
				serializer.archive(numInstances);

				// Create a draw calls for instances with the current material
				for (int instanceIndex = 0; instanceIndex < numInstances; ++instanceIndex) {
					auto& newElement = mDrawCalls.emplace_back();
					newElement.mMaterialIndex = materialIndex;

					// Create model matrix of instance and serialize it or retrieve it from the serializer
					if (serializer.mode() == gvk::serializer::mode::serialize) {
						auto instances = getModelData().mInstances;
						newElement.mModelMatrix = gvk::matrix_from_transforms(
							instances[instanceIndex].mTranslation, glm::quat(instances[instanceIndex].mRotation), instances[instanceIndex].mScaling
						);
					}
					serializer.archive(newElement.mModelMatrix);

					newElement.mPositionsBuffer = positionsBuffer;
					newElement.mIndexBuffer = indicesBuffer;
					newElement.mTexCoordsBuffer = texCoordsBuffer;
					newElement.mNormalsBuffer = normalsBuffer;
				}
			}
		}
		endPart = gvk::context().get_time();
		times.emplace_back(std::make_tuple("create materials config", endPart - startPart));
		startPart = gvk::context().get_time();

		// Convert the materials that were gathered above into a GPU-compatible format and serialize it
		// during the conversion in convert_for_gpu_usage_cached. If the serializer was initialized in
		// mode deserialize, allMatConfigs may be empty since the serializer retreives everything needed
		// from the cache file
		auto [gpuMaterials, imageSamplers] = gvk::convert_for_gpu_usage_cached(
			serializer,
			allMatConfigs, false, false,
			avk::image_usage::general_texture,
			avk::filter_mode::anisotropic_16x,
			avk::border_handling_mode::repeat,
			avk::sync::wait_idle()
		);

		endPart = gvk::context().get_time();
		times.emplace_back(std::make_tuple("convert_for_gpu_usage", endPart - startPart));
		startPart = gvk::context().get_time();

		for (auto& t : times) {
			LOG_INFO(fmt::format("{} took {}", std::get<0>(t), std::get<1>(t)));
		}

		float end = gvk::context().get_time();
		float diff = end - start;
		LOG_INFO(fmt::format("serialization/deserialization took total {}", diff));

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
			avk::vertex_shader("shaders/transform_and_pass_pos_nrm_uv.vert"),
			avk::fragment_shader("shaders/diffuse_shading_fixed_lightsource.frag"),
			// The next 3 lines define the format and location of the vertex shader inputs:
			// (The dummy values (like glm::vec3) tell the pipeline the format of the respective input)
			avk::from_buffer_binding(0) -> stream_per_vertex<glm::vec3>() -> to_location(0), // <-- corresponds to vertex shader's inPosition
			avk::from_buffer_binding(1) -> stream_per_vertex<glm::vec2>() -> to_location(1), // <-- corresponds to vertex shader's inTexCoord
			avk::from_buffer_binding(2) -> stream_per_vertex<glm::vec3>() -> to_location(2), // <-- corresponds to vertex shader's inNormal
			// Some further settings:
			avk::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(gvk::context().main_window()->backbuffer_at_index(0)),
			// We'll render to the back buffer, which has a color attachment always, and in our case additionally a depth 
			// attachment, which has been configured when creating the window. See main() function!
			avk::attachment::declare(gvk::format_from_window_color_buffer(gvk::context().main_window()), avk::on_load::clear, avk::color(0),		avk::on_store::store),	 // But not in presentable format, because ImGui comes after
			avk::attachment::declare(gvk::format_from_window_depth_buffer(gvk::context().main_window()), avk::on_load::clear, avk::depth_stencil(), avk::on_store::dont_care),
			// The following define additional data which we'll pass to the pipeline:
			//   We'll pass two matrices to our vertex shader via push constants:
			avk::push_constant_binding_data { avk::shader_type::vertex, 0, sizeof(transformation_matrices) },
			avk::descriptor_binding(0, 5, mViewProjBuffer),
			avk::descriptor_binding(4, 4, mImageSamplers),
			avk::descriptor_binding(7, 9, mMaterialBuffer) 
		);
	}
	
	void initialize() override
	{
		mInitTime = std::chrono::high_resolution_clock::now();
		
		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = gvk::context().create_descriptor_cache();
		
		mViewProjBuffer = gvk::context().create_buffer(
			avk::memory_usage::host_visible, {},
			avk::uniform_buffer_meta::create_from_data(glm::mat4())
		);
		
#ifdef USE_SERIALIZER
		load_orca_scene_cached("assets/sponza_duo.fscene");
#else
		load_orca_scene("assets/sponza_duo.fscene");
#endif

		// Add the camera to the composition (and let it handle the updates)
		mQuakeCam.set_translation({ 0.0f, 0.0f, 0.0f });
		mQuakeCam.set_perspective_projection(glm::radians(60.0f), gvk::context().main_window()->aspect_ratio(), 0.3f, 1000.0f);
		//mQuakeCam.set_orthographic_projection(-5, 5, -5, 5, 0.5, 100);
		gvk::current_composition()->add_element(mQuakeCam);

		// UI:
	    mFileBrowser.SetTitle("Select ORCA scene file");
		mFileBrowser.SetTypeFilters({ ".fscene" });
		
		auto imguiManager = gvk::current_composition()->element_by_type<gvk::imgui_manager>();
		if(nullptr != imguiManager) {
			imguiManager->add_callback([this](){
		        ImGui::Begin("Info & Settings");
				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
				ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), "[F1]: Toggle input-mode");
				ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), " (UI vs. scene navigation)");
				ImGui::DragFloat3("Rotate Objects", glm::value_ptr(mRotateObjects), 0.005f, -glm::pi<float>(), glm::pi<float>());
				ImGui::DragFloat3("Rotate Scene", glm::value_ptr(mRotateScene), 0.005f, -glm::pi<float>(), glm::pi<float>());
				ImGui::Separator();

	            if(ImGui::Button("Load ORCA scene...")) {
	                mFileBrowser.Open();
	            }		        
		        mFileBrowser.Display();
		        
		        if(mFileBrowser.HasSelected())
		        {
#ifdef USE_SERIALIZER
		            load_orca_scene_cached(mFileBrowser.GetSelected().string());
#else
		            load_orca_scene(mFileBrowser.GetSelected().string());
#endif
		            mFileBrowser.ClearSelected();
		        }

		        ImGui::End();
				
			});
		}
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

		if (gvk::input().key_pressed(gvk::key_code::h)) {
			// Log a message:
			LOG_INFO_EM("Hello cg_base!");
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
	}
	
	void render() override
	{
		auto mainWnd = gvk::context().main_window();
		if (mDestroyOldResourcesInFrame.has_value() && mDestroyOldResourcesInFrame.value() == mainWnd->current_frame()) {
			mOldDrawCalls.clear();
			mOldImageSamplers.clear();
			mOldMaterialBuffer.reset();
			mOldPipeline.reset();
			mDestroyOldResourcesInFrame.reset();
		}

		auto viewProjMat = mQuakeCam.projection_matrix() * mQuakeCam.view_matrix();
		mViewProjBuffer->fill(glm::value_ptr(viewProjMat), 0, avk::sync::not_required());
		
		auto& commandPool = gvk::context().get_command_pool_for_single_use_command_buffers(*mQueue);
		auto cmdbfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		cmdbfr->begin_recording();
		cmdbfr->begin_render_pass_for_framebuffer(mPipeline->get_renderpass(), gvk::context().main_window()->current_backbuffer());
		cmdbfr->bind_pipeline(avk::const_referenced(mPipeline));
		cmdbfr->bind_descriptors(mPipeline->layout(), mDescriptorCache.get_or_create_descriptor_sets({ 
			avk::descriptor_binding(0, 5, mViewProjBuffer),
			avk::descriptor_binding(4, 4, mImageSamplers),
			avk::descriptor_binding(7, 9, mMaterialBuffer)
		}));

		for (auto& drawCall : mDrawCalls) {
			// Set the push constants:
			auto pushConstantsForThisDrawCall = transformation_matrices {
				// Set model matrix for this mesh:
				glm::mat4{glm::orientate3(mRotateScene)} * drawCall.mModelMatrix * glm::mat4{glm::orientate3(mRotateObjects)},
				// Set material index for this mesh:
				drawCall.mMaterialIndex
			};
			cmdbfr->handle().pushConstants(mPipeline->layout_handle(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(pushConstantsForThisDrawCall), &pushConstantsForThisDrawCall);

			// Make the draw call:
			cmdbfr->draw_indexed(
				// Bind and use the index buffer:
				avk::const_referenced(drawCall.mIndexBuffer),
				// Bind the vertex input buffers in the right order (corresponding to the layout specifiers in the vertex shader)
				avk::const_referenced(drawCall.mPositionsBuffer), avk::const_referenced(drawCall.mTexCoordsBuffer), avk::const_referenced(drawCall.mNormalsBuffer)
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

private: // v== Member variables ==v

	std::chrono::high_resolution_clock::time_point mInitTime;

	avk::queue* mQueue;
	avk::descriptor_cache mDescriptorCache;

	avk::buffer mViewProjBuffer;

	std::vector<data_for_draw_call> mDrawCalls;
	std::vector<avk::image_sampler> mImageSamplers;
	avk::buffer mMaterialBuffer;
	avk::graphics_pipeline mPipeline;

	std::optional<gvk::window::frame_id_t> mDestroyOldResourcesInFrame;
	std::vector<data_for_draw_call> mOldDrawCalls;
	std::vector<avk::image_sampler> mOldImageSamplers;
	std::optional<avk::buffer> mOldMaterialBuffer;
	std::optional<avk::graphics_pipeline> mOldPipeline;
	
	gvk::quake_camera mQuakeCam;

	glm::vec3 mRotateObjects;
	glm::vec3 mRotateScene;

	ImGui::FileBrowser mFileBrowser;
	
}; // model_loader_app

int main() // <== Starting point ==
{
	try {
		// Create a window and open it
		auto mainWnd = gvk::context().create_window("ORCA Loader");
		mainWnd->set_resolution({ 1920, 1080 });
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
		auto app = orca_loader_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = gvk::imgui_manager(singleQueue);

		// GO:
		gvk::start(
			gvk::application_name("Gears-Vk + Auto-Vk Example: ORCA Loader"),
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


