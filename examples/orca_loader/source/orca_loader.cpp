#include "imgui.h"
// Use ImGui::FileBrowser from here: https://github.com/AirGuanZ/imgui-filebrowser
#include <glm/gtx/euler_angles.hpp>

#include "configure_and_compose.hpp"
#include "imfilebrowser.h"
#include "imgui_manager.hpp"
#include "invokee.hpp"
#include "material_image_helpers.hpp"
#include "model.hpp"
#include "orca_scene.hpp"
#include "serializer.hpp"
#include "sequential_invoker.hpp"
#include "orbit_camera.hpp"
#include "quake_camera.hpp"
#include "vk_convenience_functions.hpp"

#define USE_SERIALIZER 1

class orca_loader_app : public avk::invokee
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
		mDestroyOldResourcesInFrame = avk::context().main_window()->current_frame() + avk::context().main_window()->number_of_frames_in_flight(); 
		
		auto start = avk::context().get_time();
		auto startPart = start;
		auto endPart = 0.0;
		std::vector<std::tuple<std::string, double>> times;

		// Load an ORCA scene from file:
		auto orca = avk::orca_scene_t::load_from_file(aPathToOrcaScene);
		// Get all the different materials from the whole scene:
		auto distinctMaterialsOrca = orca->distinct_material_configs_for_all_models();

		endPart = avk::context().get_time();
		times.emplace_back(std::make_tuple("load orca file", endPart - startPart));
		startPart = avk::context().get_time();

		// The following loop gathers all the vertex and index data PER MATERIAL and constructs the buffers and materials.
		// Later, we'll use ONE draw call PER MATERIAL to draw the whole scene.
		std::vector<avk::material_config> allMatConfigs;
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
				auto [positionsBuffer, indicesBuffer, posIndCommands] = avk::create_vertex_and_index_buffers(
					{ avk::make_model_references_and_mesh_indices_selection(modelData.mLoadedModel, indices.mMeshIndices) }, {}
				);

				// Get a buffer containing all texture coordinates for all submeshes with this material
				auto [texCoordsBuffer, tcoCommands] = avk::create_2d_texture_coordinates_flipped_buffer<avk::vertex_buffer_meta>(
					{ avk::make_model_references_and_mesh_indices_selection(modelData.mLoadedModel, indices.mMeshIndices) }
				);

				// Get a buffer containing all normals for all submeshes with this material
				auto [normalsBuffer, nrmCommands] = avk::create_normals_buffer<avk::vertex_buffer_meta>(
					{ avk::make_model_references_and_mesh_indices_selection(modelData.mLoadedModel, indices.mMeshIndices) }
				);	

				// Submit all the fill commands to the queue:
				auto fence = avk::context().record_and_submit_with_fence({
					std::move(posIndCommands),
					std::move(tcoCommands),
					std::move(nrmCommands)
					// ^ No need for any synchronization in-between, because the commands do not depend on each other.
				}, *mQueue);

				for (size_t i = 0; i < modelData.mInstances.size(); ++i) {
					auto& newElement = mDrawCalls.emplace_back();
					newElement.mMaterialIndex = matIndex;
					newElement.mModelMatrix = avk::matrix_from_transforms(modelData.mInstances[i].mTranslation, glm::quat(modelData.mInstances[i].mRotation), modelData.mInstances[i].mScaling);
					newElement.mPositionsBuffer = positionsBuffer;
					newElement.mIndexBuffer = indicesBuffer;
					newElement.mTexCoordsBuffer = texCoordsBuffer;
					newElement.mNormalsBuffer = normalsBuffer;
				}

				// Wait on the host until the device is done:
				fence->wait_until_signalled();
			}
		}

		endPart = avk::context().get_time();
		times.emplace_back(std::make_tuple("create materials config", endPart - startPart));
		startPart = avk::context().get_time();

		// For all the different materials, transfer them in structs which are well
		// suited for GPU-usage (proper alignment, and containing only the relevant data),
		// also load all the referenced images from file and provide access to them
		// via samplers; It all happens in `ak::convert_for_gpu_usage`:
		auto [gpuMaterials, imageSamplers, materialCommands] = avk::convert_for_gpu_usage<avk::material_gpu_data>(
			allMatConfigs, false, false,
			avk::image_usage::general_texture,
			avk::filter_mode::anisotropic_16x
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

		endPart = avk::context().get_time();
		times.emplace_back(std::make_tuple("convert_for_gpu_usage and device upload", endPart - startPart));
		startPart = avk::context().get_time();

		for (auto& t : times) {
			LOG_INFO(std::format("{} took {}", std::get<0>(t), std::get<1>(t)));
		}

		auto end = avk::context().get_time();
		auto diff = end - start;
		LOG_INFO(std::format("load_orca_scene took {} in total", diff));


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
			avk::descriptor_binding(0, 5, mViewProjBuffers[0]),
			avk::descriptor_binding(4, 4, avk::as_combined_image_samplers(mImageSamplers, avk::layout::shader_read_only_optimal)),
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
		mDestroyOldResourcesInFrame = avk::context().main_window()->current_frame() + avk::context().main_window()->number_of_frames_in_flight(); 
		
		avk::orca_scene orca;
		std::unordered_map<avk::material_config, std::vector<avk::model_and_mesh_indices>> distinctMaterialsOrca;

		const std::string cacheFilePath(aPathToOrcaScene + ".cache");
		// If a cache file exists, i.e. the scene was serialized during a previous load, initialize the serializer in deserialize mode,
		// else initialize the serializer in serialize mode to create the cache file while processing the scene.
		auto serializer = avk::serializer(cacheFilePath, avk::does_cache_file_exist(cacheFilePath) ?
			avk::serializer::mode::deserialize : avk::serializer::mode::serialize);

		auto start = avk::context().get_time();
		auto startPart = start;
		auto endPart = 0.0;
		std::vector<std::tuple<std::string, double>> times;

		// Load orca scene for usage and serialization, loading the scene is not required if a cache file exists, i.e. mode == deserialize
		if (serializer.mode() == avk::serializer::mode::serialize) {
			// Load an ORCA scene from file:
			orca = avk::orca_scene_t::load_from_file(aPathToOrcaScene);
			// Get all the different materials from the whole scene:
			distinctMaterialsOrca = orca->distinct_material_configs_for_all_models();

			endPart = avk::context().get_time();
			times.emplace_back(std::make_tuple("no cache file, loading orca file", endPart - startPart));
			startPart = avk::context().get_time();
		}

		// Get number of distinc materials from orca scene and serialize it or retrieve the number from the serializer
		size_t numDistinctMaterials = (serializer.mode() == avk::serializer::mode::serialize) ? distinctMaterialsOrca.size() : 0;
		serializer.archive(numDistinctMaterials);

		// The following loop gathers all the vertex and index data PER MATERIAL and constructs the buffers and materials.
		// Later, we'll use ONE draw call PER MATERIAL to draw the whole scene.
		std::vector<avk::material_config> allMatConfigs;
		mDrawCalls.clear();
		auto materials = distinctMaterialsOrca.begin();
		for (int materialIndex = 0; materialIndex < numDistinctMaterials; ++materialIndex) {
			// meshIndices is only needed during serialization, otherwise the serializer handles everything
			// in the respective *_cached functions and meshIndices may be empty when passed to the
			// respective functions.
			size_t numMeshIndices;
			std::vector<avk::model_and_mesh_indices> meshIndices;
			if (serializer.mode() == avk::serializer::mode::serialize) {
				allMatConfigs.push_back(materials->first);
				meshIndices = materials->second;
				numMeshIndices = materials->second.size();
				materials = std::next(materials);
			}
			// Serialize or retrieve the number of model_and_mesh_indices for the material
			serializer.archive(numMeshIndices);

			for (int meshIndicesIndex = 0; meshIndicesIndex < numMeshIndices; ++meshIndicesIndex) {
				// Convinience function to retrieve the model data via the mesh indices from the orca scene while in serialize mode
				auto getModelData = [&]() -> avk::model_data& { return orca->model_at_index(meshIndices[meshIndicesIndex].mModelIndex); };

				// modelAndMeshes is only needed during serialization, otherwise the following buffers are filled by the
				// serializer from the cache file in the repsective *_cached functions and modelAndMeshes may be empty.
				std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>> modelAndMeshes;
				if (serializer.mode() == avk::serializer::mode::serialize) {
					modelAndMeshes = avk::make_model_references_and_mesh_indices_selection(getModelData().mLoadedModel, meshIndices[meshIndicesIndex].mMeshIndices);
				}

				// Get a buffer containing all positions, and one containing all indices for all submeshes with this material
				auto [positionsBuffer, indicesBuffer, posIndCommands] = avk::create_vertex_and_index_buffers_cached(serializer, modelAndMeshes, {});

				// Get a buffer containing all texture coordinates for all submeshes with this material
				auto [texCoordsBuffer, tcoCommands] = avk::create_2d_texture_coordinates_flipped_buffer_cached<avk::vertex_buffer_meta>(serializer, modelAndMeshes);

				// Get a buffer containing all normals for all submeshes with this material
				auto [normalsBuffer, nrmCommands] = avk::create_normals_buffer_cached<avk::vertex_buffer_meta>(serializer, modelAndMeshes);

				// Get the number of instances from the model and serialize it or retrieve it from the serializer
				size_t numInstances = (serializer.mode() == avk::serializer::mode::serialize) ? getModelData().mInstances.size() : 0;
				serializer.archive(numInstances);

				// Submit all the fill commands to the queue:
				auto fence = avk::context().record_and_submit_with_fence({
					std::move(posIndCommands),
					std::move(tcoCommands),
					std::move(nrmCommands)
					// ^ No need for any synchronization in-between, because the commands do not depend on each other.
				}, *mQueue);
				// Wait on the host until the device is done:
				fence->wait_until_signalled();
				
				// Create a draw calls for instances with the current material
				for (int instanceIndex = 0; instanceIndex < numInstances; ++instanceIndex) {
					auto& newElement = mDrawCalls.emplace_back();
					newElement.mMaterialIndex = materialIndex;

					// Create model matrix of instance and serialize it or retrieve it from the serializer
					if (serializer.mode() == avk::serializer::mode::serialize) {
						auto instances = getModelData().mInstances;
						newElement.mModelMatrix = avk::matrix_from_transforms(
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
		endPart = avk::context().get_time();
		times.emplace_back(std::make_tuple("create materials config", endPart - startPart));
		startPart = avk::context().get_time();

		// Convert the materials that were gathered above into a GPU-compatible format and serialize it
		// during the conversion in convert_for_gpu_usage_cached. If the serializer was initialized in
		// mode deserialize, allMatConfigs may be empty since the serializer retreives everything needed
		// from the cache file
		auto [gpuMaterials, imageSamplers, materialCommands] = avk::convert_for_gpu_usage_cached<avk::material_gpu_data>(
			serializer,
			allMatConfigs, false, false,
			avk::image_usage::general_texture,
			avk::filter_mode::anisotropic_16x
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

		endPart = avk::context().get_time();
		times.emplace_back(std::make_tuple("convert_for_gpu_usage and device upload", endPart - startPart));
		startPart = avk::context().get_time();

		for (auto& t : times) {
			LOG_INFO(std::format("{} took {}", std::get<0>(t), std::get<1>(t)));
		}

		auto end = avk::context().get_time();
		auto diff = end - start;
		LOG_INFO(std::format("load_orca_scene_cached took {} in total", diff));

		auto swapChainFormat = avk::context().main_window()->swap_chain_image_format();
		// Create our rasterization graphics pipeline with the required configuration:
		mPipeline = avk::context().create_graphics_pipeline_for(
			// Specify which shaders the pipeline consists of:
			avk::vertex_shader("shaders/transform_and_pass_pos_nrm_uv.vert"),
			avk::fragment_shader("shaders/diffuse_shading_fixed_lightsource.frag"),
			// The next 3 lines define the format and location of the vertex shader inputs:
			// (The dummy values (like glm::vec3) tell the pipeline the format of the respective input)
			avk::from_buffer_binding(0)->stream_per_vertex<glm::vec3>()->to_location(0), // <-- corresponds to vertex shader's inPosition
			avk::from_buffer_binding(1)->stream_per_vertex<glm::vec2>()->to_location(1), // <-- corresponds to vertex shader's inTexCoord
			avk::from_buffer_binding(2)->stream_per_vertex<glm::vec3>()->to_location(2), // <-- corresponds to vertex shader's inNormal
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
			avk::push_constant_binding_data{ avk::shader_type::vertex, 0, sizeof(transformation_matrices) },
			avk::descriptor_binding(0, 5, mViewProjBuffers[0]),
			avk::descriptor_binding(4, 4, avk::as_combined_image_samplers(mImageSamplers, avk::layout::shader_read_only_optimal)),
			avk::descriptor_binding(7, 9, mMaterialBuffer)
		);
	}
	
	void initialize() override
	{
		mInitTime = std::chrono::high_resolution_clock::now();
		
		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = avk::context().create_descriptor_cache();

		// One for each concurrent frame
		const auto concurrentFrames = avk::context().main_window()->number_of_frames_in_flight();
		for (int i = 0; i < concurrentFrames; ++i) {
			mViewProjBuffers.push_back(avk::context().create_buffer(
				avk::memory_usage::host_coherent, {},
				avk::uniform_buffer_meta::create_from_data(glm::mat4())
			));
		}
		
#if USE_SERIALIZER
		load_orca_scene_cached("assets/sponza_and_terrain.fscene");
#else
		load_orca_scene("assets/sponza_and_terrain.fscene");
#endif

		// Add the camera to the composition (and let it handle the updates)
		mOrbitCam.set_translation({ 0.0f, 0.0f, 0.0f });
		mQuakeCam.set_translation({ 0.0f, 0.0f, 0.0f });
		mOrbitCam.set_perspective_projection(glm::radians(60.0f), avk::context().main_window()->aspect_ratio(), 0.3f, 1000.0f);
		mQuakeCam.set_perspective_projection(glm::radians(60.0f), avk::context().main_window()->aspect_ratio(), 0.3f, 1000.0f);
		avk::current_composition()->add_element(mOrbitCam);
		avk::current_composition()->add_element(mQuakeCam);
		mQuakeCam.disable();

		// UI:
	    mFileBrowser.SetTitle("Select ORCA scene file");
		mFileBrowser.SetTypeFilters({ ".fscene" });
		
		auto imguiManager = avk::current_composition()->element_by_type<avk::imgui_manager>();
		if(nullptr != imguiManager) {
			imguiManager->add_callback([this, imguiManager] {
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
					ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), "[Esc] to exit Quake Camera navigation");
					if (avk::input().key_pressed(avk::key_code::escape)) {
						mOrbitCam.set_matrix(mQuakeCam.matrix());
						mOrbitCam.enable();
						mQuakeCam.disable();
					}
				}
				else {
					ImGui::TextColored(ImVec4(.8f, .4f, .4f, 1.f), "[Esc] to exit application");
				}
				if (imguiManager->begin_wanting_to_occupy_mouse() && mOrbitCam.is_enabled()) {
					mOrbitCam.disable();
				}
				if (imguiManager->end_wanting_to_occupy_mouse() && !mQuakeCam.is_enabled()) {
					mOrbitCam.enable();
				}
				ImGui::Separator();

				ImGui::DragFloat3("Rotate Objects", glm::value_ptr(mRotateObjects), 0.005f, -glm::pi<float>(), glm::pi<float>());
				ImGui::DragFloat3("Rotate Scene", glm::value_ptr(mRotateScene), 0.005f, -glm::pi<float>(), glm::pi<float>());
				ImGui::Separator();

	            if(ImGui::Button("Load ORCA scene...")) {
	                mFileBrowser.Open();
	            }		        
		        mFileBrowser.Display();
		        
		        if(mFileBrowser.HasSelected())
		        {
#if USE_SERIALIZER
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

		if (avk::input().key_pressed(avk::key_code::c)) {
			// Center the cursor:
			auto resolution = avk::context().main_window()->resolution();
			avk::context().main_window()->set_cursor_pos({ resolution[0] / 2.0, resolution[1] / 2.0 });
		}
		if (!mQuakeCam.is_enabled() && avk::input().key_pressed(avk::key_code::escape) || avk::context().main_window()->should_be_closed()) {
			// Stop the current composition:
			avk::current_composition()->stop();
		}
	}
	
	void render() override
	{
		auto mainWnd = avk::context().main_window();
		auto ifi = mainWnd->current_in_flight_index();

		if (mDestroyOldResourcesInFrame.has_value() && mDestroyOldResourcesInFrame.value() == mainWnd->current_frame()) {
			mOldDrawCalls.clear();
			mOldImageSamplers.clear();
			mOldMaterialBuffer.reset();
			mOldPipeline.reset();
			mDestroyOldResourcesInFrame.reset();
		}

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
						avk::descriptor_binding(0, 5, mViewProjBuffers[ifi]),
						avk::descriptor_binding(4, 4, avk::as_combined_image_samplers(mImageSamplers, avk::layout::shader_read_only_optimal)),
						avk::descriptor_binding(7, 9, mMaterialBuffer)
					})),

					// Draw all the draw calls:
					avk::command::custom_commands([&,this](avk::command_buffer_t& cb) { // If there is no avk::command::... struct for a particular command, we can always use avk::command::custom_commands
						for (auto& drawCall : mDrawCalls) {
							// Set the push constants per draw call:
							cb.record({
								avk::command::push_constants(
									mPipeline->layout(),
									transformation_matrices{
										// Set model matrix for this mesh:
										glm::mat4{glm::orientate3(mRotateScene)} *drawCall.mModelMatrix * glm::mat4{glm::orientate3(mRotateObjects)},
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

private: // v== Member variables ==v

	std::chrono::high_resolution_clock::time_point mInitTime;

	avk::queue* mQueue;
	avk::descriptor_cache mDescriptorCache;

	std::vector<avk::buffer> mViewProjBuffers;

	std::vector<data_for_draw_call> mDrawCalls;
	std::vector<avk::image_sampler> mImageSamplers;
	avk::buffer mMaterialBuffer;
	avk::graphics_pipeline mPipeline;

	std::optional<avk::window::frame_id_t> mDestroyOldResourcesInFrame;
	std::vector<data_for_draw_call> mOldDrawCalls;
	std::vector<avk::image_sampler> mOldImageSamplers;
	std::optional<avk::buffer> mOldMaterialBuffer;
	std::optional<avk::graphics_pipeline> mOldPipeline;
	
	avk::orbit_camera mOrbitCam;
	avk::quake_camera mQuakeCam;

	glm::vec3 mRotateObjects = { 0.f, 0.f, 0.f };
	glm::vec3 mRotateScene = { 0.f, 0.f, 0.f };

	ImGui::FileBrowser mFileBrowser;
	
};

int main() // <== Starting point ==
{
	int result = EXIT_FAILURE;
	try {
		// Create a window and open it
		auto mainWnd = avk::context().create_window("ORCA Loader");
		mainWnd->set_resolution({ 1920, 1080 });
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
		auto app = orca_loader_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = avk::imgui_manager(singleQueue);
		ui.set_custom_font("assets/JetBrainsMono-Regular.ttf");

		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			avk::application_name("Auto-Vk-Toolkit Example: ORCA Loader"),
			[](avk::validation_layers& config) {
				//config.enable_feature(vk::ValidationFeatureEnableEXT::eSynchronizationValidation);
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


