#include <cg_base.hpp>
#include <imgui.h>
#include <glm/gtx/euler_angles.hpp>

class orca_loader_app : public cgb::cg_element
{
	struct data_for_draw_call
	{
		cgb::vertex_buffer mPositionsBuffer;
		cgb::vertex_buffer mTexCoordsBuffer;
		cgb::vertex_buffer mNormalsBuffer;
		cgb::index_buffer mIndexBuffer;
		int mMaterialIndex;
		glm::mat4 mModelMatrix;
	};

	struct transformation_matrices {
		glm::mat4 mModelMatrix;
		int mMaterialIndex;
	};

public: // v== cgb::cg_element overrides which will be invoked by the framework ==v

	void initialize() override
	{
		mInitTime = std::chrono::high_resolution_clock::now();

		// Load a model from file:
		auto sponza = cgb::model_t::load_from_file("assets/sponza_structure.obj", aiProcess_Triangulate | aiProcess_PreTransformVertices);
		// Get all the different materials of the model:
		auto distinctMaterialsSponza = sponza->distinct_material_configs();

		// Load an ORCA scene from file:
		auto orca = cgb::orca_scene_t::load_from_file("assets/sponza_duo.fscene");
		// Get all the different materials from the whole scene:
		auto distinctMaterialsOrca = orca->distinct_material_configs_for_all_models();

		// Merge them all together:

		// The following loop gathers all the vertex and index data PER MATERIAL and constructs the buffers and materials.
		// Later, we'll use ONE draw call PER MATERIAL to draw the whole scene.
		std::vector<cgb::material_config> allMatConfigs;
		for (const auto& pair : distinctMaterialsSponza) {
			allMatConfigs.push_back(pair.first);
			auto matIndex = allMatConfigs.size() - 1;

			auto& newElement = mDrawCalls.emplace_back();
			newElement.mMaterialIndex = static_cast<int>(matIndex);
			newElement.mModelMatrix = glm::scale(glm::vec3(0.01f));
			
			// Compared to the "model_loader" example, we are taking a more optimistic appproach here.
			// By not using `cgb::append_indices_and_vertex_data` directly, we have no guarantee that
			// all vectors of vertex-data are of the same length. 
			// Instead, here we use the (possibly more convenient) `cgb::get_combined*` functions and
			// just optimistically assume that positions, texture coordinates, and normals are all of
			// the same length.

			// Get a buffer containing all positions, and one containing all indices for all submeshes with this material
			auto [positionsBuffer, indicesBuffer] = cgb::create_vertex_and_index_buffers(
				{ cgb::make_models_and_meshes_selection(sponza, pair.second) }, 
				cgb::sync::with_semaphore_to_backbuffer_dependency()
			);
			newElement.mPositionsBuffer = std::move(positionsBuffer);
			newElement.mIndexBuffer = std::move(indicesBuffer);

			// Get a buffer containing all texture coordinates for all submeshes with this material
			newElement.mTexCoordsBuffer = cgb::create_2d_texture_coordinates_buffer(
				{ cgb::make_models_and_meshes_selection(sponza, pair.second) }, 0,
				cgb::sync::with_semaphore_to_backbuffer_dependency()
			);

			// Get a buffer containing all normals for all submeshes with this material
			newElement.mNormalsBuffer = cgb::create_normals_buffer(
				{ cgb::make_models_and_meshes_selection(sponza, pair.second) }, 
				cgb::sync::with_semaphore_to_backbuffer_dependency()
			);
		}
		// Same for the ORCA scene:
		for (const auto& pair : distinctMaterialsOrca) {
			// See if we've already encountered this material in the loop above
			auto it = std::find(std::begin(allMatConfigs), std::end(allMatConfigs), pair.first);
			size_t matIndex;
			if (allMatConfigs.end() == it) {
				// Couldn't find => insert new material
				allMatConfigs.push_back(pair.first);
				matIndex = allMatConfigs.size() - 1;
			}
			else {
				// Found the material => use the existing material
				matIndex = std::distance(std::begin(allMatConfigs), it);
			}

			// The data in distinctMaterialsOrca encompasses all of the ORCA scene's models.
			for (const auto& indices : pair.second) {
				// However, we have to pay attention to the specific model's scene-properties,...
				auto& modelData = orca->model_at_index(indices.mModelIndex);
				// ... specifically, to its instances:
				
				// (Generally, we can't combine the vertex data in this case with the vertex
				//  data of other models if we have to draw multiple instances; because in 
				//  the case of multiple instances, only the to-be-instanced geometry must
				//  be accessible independently of the other geometry.
				//  Therefore, in this example, we take the approach of building separate 
				//  buffers for everything which could potentially be instanced.)

				// Get a buffer containing all positions, and one containing all indices for all submeshes with this material
				auto [positionsBuffer, indicesBuffer] = cgb::create_vertex_and_index_buffers(
					{ cgb::make_models_and_meshes_selection(modelData.mLoadedModel, indices.mMeshIndices) }, 
					cgb::sync::with_semaphore_to_backbuffer_dependency()
				);
				positionsBuffer.enable_shared_ownership(); // Enable multiple owners of this buffer, because there might be multiple model-instances and hence, multiple draw calls that want to use this buffer.
				indicesBuffer.enable_shared_ownership(); // Enable multiple owners of this buffer, because there might be multiple model-instances and hence, multiple draw calls that want to use this buffer.

				// Get a buffer containing all texture coordinates for all submeshes with this material
				auto texCoordsBuffer = cgb::create_2d_texture_coordinates_buffer(
					{ cgb::make_models_and_meshes_selection(modelData.mLoadedModel, indices.mMeshIndices) }, 0,
					cgb::sync::with_semaphore_to_backbuffer_dependency()
				);
				texCoordsBuffer.enable_shared_ownership(); // Enable multiple owners of this buffer, because there might be multiple model-instances and hence, multiple draw calls that want to use this buffer.

				// Get a buffer containing all normals for all submeshes with this material
				auto normalsBuffer = cgb::create_normals_buffer(
					{ cgb::make_models_and_meshes_selection(modelData.mLoadedModel, indices.mMeshIndices) }, 
					cgb::sync::with_semaphore_to_backbuffer_dependency()
				);
				normalsBuffer.enable_shared_ownership(); // Enable multiple owners of this buffer, because there might be multiple model-instances and hence, multiple draw calls that want to use this buffer.

				for (size_t i = 0; i < modelData.mInstances.size(); ++i) {
					auto& newElement = mDrawCalls.emplace_back();
					newElement.mMaterialIndex = static_cast<int>(matIndex);
					newElement.mModelMatrix = cgb::matrix_from_transforms(modelData.mInstances[i].mTranslation, glm::quat(modelData.mInstances[i].mRotation), modelData.mInstances[i].mScaling);
					newElement.mPositionsBuffer = positionsBuffer;
					newElement.mIndexBuffer = indicesBuffer;
					newElement.mTexCoordsBuffer = texCoordsBuffer;
					newElement.mNormalsBuffer = normalsBuffer;
				}
			}
		}

		// Convert the materials that were gathered above into a GPU-compatible format, and upload into a GPU storage buffer:
		auto [gpuMaterials, imageSamplers] = cgb::convert_for_gpu_usage(
			allMatConfigs, 
			xv::image_usage::general_texture,
			cgb::filter_mode::anisotropic_16x,
			cgb::border_handling_mode::repeat,
			cgb::sync::with_semaphore_to_backbuffer_dependency()
		);

		mViewProjBuffer = cgb::create(
			cgb::uniform_buffer_meta::create_from_data(glm::mat4()),
			xv::memory_usage::host_coherent
		);
		
		mMaterialBuffer = cgb::create_and_fill(
			cgb::storage_buffer_meta::create_from_data(gpuMaterials),
			xv::memory_usage::host_coherent,
			gpuMaterials.data(),
			cgb::sync::not_required()
		);
		mImageSamplers = std::move(imageSamplers);

		auto swapChainFormat = cgb::context().main_window()->swap_chain_image_format();
		// Create our rasterization graphics pipeline with the required configuration:
		mPipeline = cgb::graphics_pipeline_for(
			// Specify which shaders the pipeline consists of:
			cgb::vertex_shader("shaders/transform_and_pass_pos_nrm_uv.vert"),
			cgb::fragment_shader("shaders/diffuse_shading_fixed_lightsource.frag"),
			// The next 3 lines define the format and location of the vertex shader inputs:
			// (The dummy values (like glm::vec3) tell the pipeline the format of the respective input)
			cgb::vertex_input_location(0, glm::vec3{}).from_buffer_at_binding(0), // <-- corresponds to vertex shader's inPosition
			cgb::vertex_input_location(1, glm::vec2{}).from_buffer_at_binding(1), // <-- corresponds to vertex shader's inTexCoord
			cgb::vertex_input_location(2, glm::vec3{}).from_buffer_at_binding(2), // <-- corresponds to vertex shader's inNormal
			// Some further settings:
			cgb::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			cgb::cfg::viewport_depth_scissors_config::from_window(cgb::context().main_window()),
			// We'll render to the back buffer, which has a color attachment always, and in our case additionally a depth 
			// attachment, which has been configured when creating the window. See main() function!
			xv::attachment::declare(cgb::image_format::from_window_color_buffer(),	xv::on_load::clear, xv::color(0),		 xv::on_store::store),		 // But not in presentable format, because ImGui comes after
			xv::attachment::declare(cgb::image_format::from_window_depth_buffer(),	xv::on_load::clear, xv::depth_stencil(), xv::on_store::dont_care),
			// The following define additional data which we'll pass to the pipeline:
			//   We'll pass two matrices to our vertex shader via push constants:
			cgb::push_constant_binding_data { cgb::shader_type::vertex, 0, sizeof(transformation_matrices) },
			cgb::binding(0, 5, mViewProjBuffer),
			cgb::binding(4, 4, mImageSamplers),
			cgb::binding(7, 9, mMaterialBuffer) 
		);

		// Add the camera to the composition (and let it handle the updates)
		mQuakeCam.set_translation({ 0.0f, 0.0f, 0.0f });
		mQuakeCam.set_perspective_projection(glm::radians(60.0f), cgb::context().main_window()->aspect_ratio(), 0.5f, 100.0f);
		//mQuakeCam.set_orthographic_projection(-5, 5, -5, 5, 0.5, 100);
		cgb::current_composition().add_element(mQuakeCam);

		auto imguiManager = cgb::current_composition().element_by_type<cgb::imgui_manager>();
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
				ImGui::End();
			});
		}
	}

	void render() override
	{
		auto mainWnd = cgb::context().main_window();

		auto viewProjMat = mQuakeCam.projection_matrix() * mQuakeCam.view_matrix();
		cgb::fill(mViewProjBuffer, glm::value_ptr(viewProjMat), cgb::sync::not_required());
		
		auto cmdbfr = cgb::command_pool::create_single_use_command_buffer(cgb::context().graphics_queue());
		cmdbfr->begin_recording();
		cmdbfr->begin_render_pass_for_framebuffer(mPipeline->get_renderpass(), mainWnd->backbufer_for_frame());
		cmdbfr->bind_pipeline(mPipeline);
		cmdbfr->bind_descriptors(mPipeline->layout(), {
				cgb::binding(0, 5, mViewProjBuffer),
				cgb::binding(4, 4, mImageSamplers),
				cgb::binding(7, 9, mMaterialBuffer)
			}
		);

		for (auto& drawCall : mDrawCalls) {
			// Bind the vertex input buffers in the right order (corresponding to the layout specifiers in the vertex shader)
			cmdbfr->handle().bindVertexBuffers(0u, {
				drawCall.mPositionsBuffer->buffer_handle(), 
				drawCall.mTexCoordsBuffer->buffer_handle(), 
				drawCall.mNormalsBuffer->buffer_handle()
			}, { 0, 0, 0 });

			// Set the push constants:
			auto pushConstantsForThisDrawCall = transformation_matrices {
				// Set model matrix for this mesh:
				glm::mat4{glm::orientate3(mRotateScene)} * drawCall.mModelMatrix * glm::mat4{glm::orientate3(mRotateObjects)},
				// Set material index for this mesh:
				drawCall.mMaterialIndex
			};
			cmdbfr->handle().pushConstants(mPipeline->layout_handle(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(pushConstantsForThisDrawCall), &pushConstantsForThisDrawCall);

			// Bind and use the index buffer to create the draw call:
			vk::IndexType indexType = cgb::to_vk_index_type(drawCall.mIndexBuffer->meta_data().sizeof_one_element());
			cmdbfr->handle().bindIndexBuffer(drawCall.mIndexBuffer->buffer_handle(), 0u, indexType);
			cmdbfr->handle().drawIndexed(drawCall.mIndexBuffer->meta_data().num_elements(), 1u, 0u, 0u, 0u);
		}

		cmdbfr->end_render_pass();
		cmdbfr->end_recording();
		mainWnd->submit_for_backbuffer(std::move(cmdbfr));
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

		if (cgb::input().key_pressed(cgb::key_code::space)) {
			// Print the current camera position
			auto pos = mQuakeCam.translation();
			LOG_INFO(fmt::format("Current camera position: {}", cgb::to_string(pos)));
		}
		if (cgb::input().key_pressed(cgb::key_code::escape)) {
			// Stop the current composition:
			cgb::current_composition().stop();
		}
		if (cgb::input().key_pressed(cgb::key_code::f1)) {
			auto imguiManager = cgb::current_composition().element_by_type<cgb::imgui_manager>();
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
	
	void finalize() override
	{
		cgb::context().logical_device().waitIdle();
	}

private: // v== Member variables ==v

	std::chrono::high_resolution_clock::time_point mInitTime;

	cgb::uniform_buffer mViewProjBuffer;
	cgb::storage_buffer mMaterialBuffer;
	std::vector<cgb::image_sampler> mImageSamplers;

	std::vector<data_for_draw_call> mDrawCalls;
	std::shared_ptr<cgb::descriptor_set> mDescriptorSet;
	cgb::graphics_pipeline mPipeline;
	cgb::quake_camera mQuakeCam;

	glm::vec3 mRotateObjects;
	glm::vec3 mRotateScene;
	
}; // model_loader_app

int main() // <== Starting point ==
{
	try {
		// What's the name of our application
		//cgb::settings::gPhysicalDeviceSelectionHint = "radeon";
		cgb::settings::gApplicationName = "cg_base::orca_loader";
		cgb::settings::gLoadImagesInSrgbFormatByDefault = true;
		cgb::settings::gQueueSelectionPreference = cgb::device_queue_selection_strategy::prefer_everything_on_single_queue;

		// Create a window and open it
		auto mainWnd = cgb::context().create_window("cg_base: ORCA Loader Example");
		mainWnd->set_resolution({ 1920, 1080 });
		mainWnd->set_presentaton_mode(cgb::presentation_mode::mailbox);
		mainWnd->set_additional_back_buffer_attachments({ 
			cgb::attachment::declare(cgb::image_format::default_depth_format(), cgb::xv::on_load::clear, cgb::xv::depth_stencil(), cgb::xv::on_store::dont_care)
		});
		mainWnd->request_srgb_framebuffer(true);
		mainWnd->open(); 

		// Create an instance of our main cgb::element which contains all the functionality:
		auto app = orca_loader_app();
		// Create another element for drawing the UI with ImGui
		auto ui = cgb::imgui_manager();

		auto orcaLoader = cgb::setup(app, ui);
		orcaLoader.start();
	}
	catch (cgb::logic_error&) {}
	catch (cgb::runtime_error&) {}
}


