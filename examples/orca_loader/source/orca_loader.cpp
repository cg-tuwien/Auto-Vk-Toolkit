#include <exekutor.hpp>
#include <imgui.h>
#include <glm/gtx/euler_angles.hpp>

class orca_loader_app : public xk::invokee
{
	struct data_for_draw_call
	{
		ak::buffer mPositionsBuffer;
		ak::buffer mTexCoordsBuffer;
		ak::buffer mNormalsBuffer;
		ak::buffer mIndexBuffer;
		int mMaterialIndex;
		glm::mat4 mModelMatrix;
	};

	struct transformation_matrices {
		glm::mat4 mModelMatrix;
		int mMaterialIndex;
	};

public: // v== ak::invokee overrides which will be invoked by the framework ==v
	orca_loader_app(ak::queue& aQueue)
		: mQueue{ &aQueue }
	{}
	
	void initialize() override
	{
		mInitTime = std::chrono::high_resolution_clock::now();
		
		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = xk::context().create_descriptor_cache();
		
		// Load a model from file:
		auto sponza = xk::model_t::load_from_file("assets/sponza_structure.obj", aiProcess_Triangulate | aiProcess_PreTransformVertices);
		// Get all the different materials of the model:
		auto distinctMaterialsSponza = sponza->distinct_material_configs();

		// Load an ORCA scene from file:
		auto orca = xk::orca_scene_t::load_from_file("assets/sponza_duo.fscene");
		// Get all the different materials from the whole scene:
		auto distinctMaterialsOrca = orca->distinct_material_configs_for_all_models();

		// Merge them all together:

		// The following loop gathers all the vertex and index data PER MATERIAL and constructs the buffers and materials.
		// Later, we'll use ONE draw call PER MATERIAL to draw the whole scene.
		std::vector<xk::material_config> allMatConfigs;
		for (const auto& pair : distinctMaterialsSponza) {
			allMatConfigs.push_back(pair.first);
			auto matIndex = allMatConfigs.size() - 1;

			auto& newElement = mDrawCalls.emplace_back();
			newElement.mMaterialIndex = static_cast<int>(matIndex);
			newElement.mModelMatrix = glm::scale(glm::vec3(0.01f));
			
			// Compared to the "model_loader" example, we are taking a more optimistic appproach here.
			// By not using `ak::append_indices_and_vertex_data` directly, we have no guarantee that
			// all vectors of vertex-data are of the same length. 
			// Instead, here we use the (possibly more convenient) `ak::get_combined*` functions and
			// just optimistically assume that positions, texture coordinates, and normals are all of
			// the same length.

			// Get a buffer containing all positions, and one containing all indices for all submeshes with this material
			auto [positionsBuffer, indicesBuffer] = xk::create_vertex_and_index_buffers(
				{ xk::make_models_and_meshes_selection(sponza, pair.second) }, {}, 
				ak::sync::wait_idle()
			);
			newElement.mPositionsBuffer = std::move(positionsBuffer);
			newElement.mIndexBuffer = std::move(indicesBuffer);

			// Get a buffer containing all texture coordinates for all submeshes with this material
			newElement.mTexCoordsBuffer = xk::create_2d_texture_coordinates_buffer(
				{ xk::make_models_and_meshes_selection(sponza, pair.second) }, 0,
				ak::sync::wait_idle()
			);

			// Get a buffer containing all normals for all submeshes with this material
			newElement.mNormalsBuffer = xk::create_normals_buffer(
				{ xk::make_models_and_meshes_selection(sponza, pair.second) }, 
				ak::sync::wait_idle()
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
				auto [positionsBuffer, indicesBuffer] = xk::create_vertex_and_index_buffers(
					{ xk::make_models_and_meshes_selection(modelData.mLoadedModel, indices.mMeshIndices) }, {},
					ak::sync::wait_idle()
				);
				positionsBuffer.enable_shared_ownership(); // Enable multiple owners of this buffer, because there might be multiple model-instances and hence, multiple draw calls that want to use this buffer.
				indicesBuffer.enable_shared_ownership(); // Enable multiple owners of this buffer, because there might be multiple model-instances and hence, multiple draw calls that want to use this buffer.

				// Get a buffer containing all texture coordinates for all submeshes with this material
				auto texCoordsBuffer = xk::create_2d_texture_coordinates_buffer(
					{ xk::make_models_and_meshes_selection(modelData.mLoadedModel, indices.mMeshIndices) }, 0,
					ak::sync::wait_idle()
				);
				texCoordsBuffer.enable_shared_ownership(); // Enable multiple owners of this buffer, because there might be multiple model-instances and hence, multiple draw calls that want to use this buffer.

				// Get a buffer containing all normals for all submeshes with this material
				auto normalsBuffer = xk::create_normals_buffer(
					{ xk::make_models_and_meshes_selection(modelData.mLoadedModel, indices.mMeshIndices) }, 
					ak::sync::wait_idle()
				);
				normalsBuffer.enable_shared_ownership(); // Enable multiple owners of this buffer, because there might be multiple model-instances and hence, multiple draw calls that want to use this buffer.

				for (size_t i = 0; i < modelData.mInstances.size(); ++i) {
					auto& newElement = mDrawCalls.emplace_back();
					newElement.mMaterialIndex = static_cast<int>(matIndex);
					newElement.mModelMatrix = xk::matrix_from_transforms(modelData.mInstances[i].mTranslation, glm::quat(modelData.mInstances[i].mRotation), modelData.mInstances[i].mScaling);
					newElement.mPositionsBuffer = positionsBuffer;
					newElement.mIndexBuffer = indicesBuffer;
					newElement.mTexCoordsBuffer = texCoordsBuffer;
					newElement.mNormalsBuffer = normalsBuffer;
				}
			}
		}

		// Convert the materials that were gathered above into a GPU-compatible format, and upload into a GPU storage buffer:
		auto [gpuMaterials, imageSamplers] = xk::convert_for_gpu_usage(
			allMatConfigs, false,
			ak::image_usage::general_texture,
			ak::filter_mode::anisotropic_16x,
			ak::border_handling_mode::repeat,
			ak::sync::wait_idle()
		);

		mViewProjBuffer = xk::context().create_buffer(
			ak::memory_usage::host_coherent, {},
			ak::uniform_buffer_meta::create_from_data(glm::mat4())
		);
		
		mMaterialBuffer = xk::context().create_buffer(
			ak::memory_usage::host_coherent, {},
			ak::storage_buffer_meta::create_from_data(gpuMaterials)
		);
		mMaterialBuffer->fill(
			gpuMaterials.data(), 0,
			ak::sync::not_required()
		);
		mImageSamplers = std::move(imageSamplers);

		auto swapChainFormat = xk::context().main_window()->swap_chain_image_format();
		// Create our rasterization graphics pipeline with the required configuration:
		mPipeline = xk::context().create_graphics_pipeline_for(
			// Specify which shaders the pipeline consists of:
			ak::vertex_shader("shaders/transform_and_pass_pos_nrm_uv.vert"),
			ak::fragment_shader("shaders/diffuse_shading_fixed_lightsource.frag"),
			// The next 3 lines define the format and location of the vertex shader inputs:
			// (The dummy values (like glm::vec3) tell the pipeline the format of the respective input)
			ak::vertex_input_location(0, glm::vec3{}).from_buffer_at_binding(0), // <-- corresponds to vertex shader's inPosition
			ak::vertex_input_location(1, glm::vec2{}).from_buffer_at_binding(1), // <-- corresponds to vertex shader's inTexCoord
			ak::vertex_input_location(2, glm::vec3{}).from_buffer_at_binding(2), // <-- corresponds to vertex shader's inNormal
			// Some further settings:
			ak::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
			ak::cfg::viewport_depth_scissors_config::from_framebuffer(xk::context().main_window()->backbuffer_at_index(0)),
			// We'll render to the back buffer, which has a color attachment always, and in our case additionally a depth 
			// attachment, which has been configured when creating the window. See main() function!
			ak::attachment::declare(xk::format_from_window_color_buffer(xk::context().main_window()), ak::on_load::clear, ak::color(0),		ak::on_store::store),	 // But not in presentable format, because ImGui comes after
			ak::attachment::declare(xk::from_window_depth_buffer(xk::context().main_window()), ak::on_load::clear, ak::depth_stencil(), ak::on_store::dont_care),
			// The following define additional data which we'll pass to the pipeline:
			//   We'll pass two matrices to our vertex shader via push constants:
			ak::push_constant_binding_data { ak::shader_type::vertex, 0, sizeof(transformation_matrices) },
			ak::binding(0, 5, mViewProjBuffer),
			ak::binding(4, 4, mImageSamplers),
			ak::binding(7, 9, mMaterialBuffer) 
		);

		// Add the camera to the composition (and let it handle the updates)
		mQuakeCam.set_translation({ 0.0f, 0.0f, 0.0f });
		mQuakeCam.set_perspective_projection(glm::radians(60.0f), xk::context().main_window()->aspect_ratio(), 0.5f, 100.0f);
		//mQuakeCam.set_orthographic_projection(-5, 5, -5, 5, 0.5, 100);
		xk::current_composition()->add_element(mQuakeCam);

		auto imguiManager = xk::current_composition()->element_by_type<xk::imgui_manager>();
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
		auto mainWnd = xk::context().main_window();

		auto viewProjMat = mQuakeCam.projection_matrix() * mQuakeCam.view_matrix();
		mViewProjBuffer->fill(glm::value_ptr(viewProjMat), 0, ak::sync::not_required());
		
		auto& commandPool = xk::context().get_command_pool_for_single_use_command_buffers(*mQueue);
		auto cmdbfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		cmdbfr->begin_recording();
		cmdbfr->begin_render_pass_for_framebuffer(mPipeline->get_renderpass(), xk::context().main_window()->current_backbuffer());
		cmdbfr->bind_pipeline(mPipeline);
		cmdbfr->bind_descriptors(mPipeline->layout(), mDescriptorCache.get_or_create_descriptor_sets({ 
			ak::binding(0, 5, mViewProjBuffer),
			ak::binding(4, 4, mImageSamplers),
			ak::binding(7, 9, mMaterialBuffer)
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
				*drawCall.mIndexBuffer,
				// Bind the vertex input buffers in the right order (corresponding to the layout specifiers in the vertex shader)
				*drawCall.mPositionsBuffer, *drawCall.mTexCoordsBuffer, *drawCall.mNormalsBuffer
			);
		}

		cmdbfr->end_render_pass();
		cmdbfr->end_recording();

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto& imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();
		
		// Submit the draw call and take care of the command buffer's lifetime:
		mQueue->submit(cmdbfr, imageAvailableSemaphore);
		mainWnd->handle_lifetime(std::move(cmdbfr));
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

		if (xk::input().key_pressed(xk::key_code::h)) {
			// Log a message:
			LOG_INFO_EM("Hello cg_base!");
		}
		if (xk::input().key_pressed(xk::key_code::c)) {
			// Center the cursor:
			auto resolution = xk::context().main_window()->resolution();
			xk::context().main_window()->set_cursor_pos({ resolution[0] / 2.0, resolution[1] / 2.0 });
		}
		if (xk::input().key_pressed(xk::key_code::escape)) {
			// Stop the current composition:
			xk::current_composition()->stop();
		}
		if (xk::input().key_pressed(xk::key_code::f1)) {
			auto imguiManager = xk::current_composition()->element_by_type<xk::imgui_manager>();
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

	std::chrono::high_resolution_clock::time_point mInitTime;

	ak::queue* mQueue;
	ak::descriptor_cache mDescriptorCache;

	ak::buffer mViewProjBuffer;
	ak::buffer mMaterialBuffer;
	std::vector<ak::image_sampler> mImageSamplers;

	std::vector<data_for_draw_call> mDrawCalls;
	ak::graphics_pipeline mPipeline;
	xk::quake_camera mQuakeCam;

	glm::vec3 mRotateObjects;
	glm::vec3 mRotateScene;
	
}; // model_loader_app

int main() // <== Starting point ==
{
	try {
		// Create a window and open it
		auto mainWnd = xk::context().create_window("ORCA Loader");
		mainWnd->set_resolution({ 1920, 1080 });
		mainWnd->set_additional_back_buffer_attachments({ 
			ak::attachment::declare(vk::Format::eD32Sfloat, ak::on_load::clear, ak::depth_stencil(), ak::on_store::dont_care)
		});
		mainWnd->set_presentaton_mode(xk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = xk::context().create_queue({}, ak::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->add_queue_family_ownership(singleQueue);
		mainWnd->set_present_queue(singleQueue);
		
		// Create an instance of our main ak::element which contains all the functionality:
		auto app = orca_loader_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = xk::imgui_manager(singleQueue);

		// GO:
		xk::execute(
			xk::application_name("Exekutor + Auto-Vk Example: ORCA Loader"),
			mainWnd,
			app,
			ui
		);
	}
	catch (xk::logic_error&) {}
	catch (xk::runtime_error&) {}
	catch (ak::logic_error&) {}
	catch (ak::runtime_error&) {}
}


