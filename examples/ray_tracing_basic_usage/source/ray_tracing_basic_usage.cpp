#include <cg_base.hpp>
#include <imgui.h>

class ray_tracing_basic_usage_app : public cgb::cg_element
{
	struct push_const_data {
		glm::mat4 mViewMatrix;
		glm::vec4 mLightDirection;
	};

public: // v== cgb::cg_element overrides which will be invoked by the framework ==v

	void initialize() override
	{
		// Load an ORCA scene from file:
		auto orca = cgb::orca_scene_t::load_from_file("assets/sponza_duo.fscene");
		// Iterate over all models, all model instances, and all meshes, and create bottom level acceleration structures for each one of them:
		for (const auto& modelData : orca->models()) {
			for (const auto& modelInstance : modelData.mInstances) {
				const auto& model = modelData.mLoadedModel;
				auto meshIndices = model->select_all_meshes();
				auto [vtxBfr, idxBfr] = cgb::create_vertex_and_index_buffers({ cgb::make_models_and_meshes_selection(model, meshIndices) });
				auto blas = cgb::bottom_level_acceleration_structure_t::create({ cgb::acceleration_structure_size_requirements::from_buffers(vtxBfr, idxBfr) }, false);
				std::vector<std::tuple<std::reference_wrapper<const cgb::vertex_buffer_t>, std::reference_wrapper<const cgb::index_buffer_t>>> schass;
				schass.emplace_back(std::cref(*vtxBfr), std::cref(*idxBfr));
				blas->build(schass);
				mGeometryInstances.push_back(
					cgb::geometry_instance::create(blas)
						.set_transform(cgb::matrix_from_transforms(modelInstance.mTranslation, glm::quat(modelInstance.mRotation), modelInstance.mScaling))
				);
				mBLASs.push_back(std::move(blas));
			}
		}
		
		cgb::invoke_for_all_in_flight_frames(cgb::context().main_window(), [&](auto inFlightIndex){
			auto tlas = cgb::top_level_acceleration_structure_t::create(mGeometryInstances.size());
			tlas->build(mGeometryInstances);
			mTLAS.push_back(std::move(tlas));
		});
		
		// Create offscreen image views to ray-trace into, one for each frame in flight:
		size_t n = cgb::context().main_window()->number_of_in_flight_frames();
		const auto wdth = cgb::context().main_window()->resolution().x;
		const auto hght = cgb::context().main_window()->resolution().y;
		const auto frmt = cgb::image_format::from_window_color_buffer(cgb::context().main_window());
		cgb::invoke_for_all_in_flight_frames(cgb::context().main_window(), [&](auto inFlightIndex){
			mOffscreenImageViews.emplace_back(
				cgb::image_view_t::create(
					cgb::image_t::create(wdth, hght, frmt, 1, cgb::memory_usage::device, cgb::image_usage::general_storage_image)
				)
			);
			mOffscreenImageViews.back()->get_image().transition_to_layout({}, cgb::sync::with_barriers(cgb::context().main_window()->command_buffer_lifetime_handler()));
			assert((mOffscreenImageViews.back()->config().subresourceRange.aspectMask & vk::ImageAspectFlagBits::eColor) == vk::ImageAspectFlagBits::eColor);
		});

		// Create our ray tracing pipeline with the required configuration:
		mPipeline = cgb::ray_tracing_pipeline_for(
			cgb::define_shader_table(
				cgb::ray_generation_shader("shaders/ray_generation_shader.rgen"),
				cgb::triangles_hit_group::create_with_rchit_only("shaders/closest_hit_shader.rchit"),
				cgb::miss_shader("shaders/miss_shader.rmiss")
			),
			cgb::max_recursion_depth::set_to_max(),
			// Define push constants and descriptor bindings:
			cgb::push_constant_binding_data { cgb::shader_type::ray_generation | cgb::shader_type::closest_hit, 0, sizeof(push_const_data) },
			cgb::binding(0, 0, mOffscreenImageViews[0]), // Just take any, this is just to define the layout
			cgb::binding(1, 0, mTLAS[0])				 // Just take any, this is just to define the layout
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
				ImGui::DragFloat3("Light Direction", glm::value_ptr(mLightDir), 0.005f, -1.0f, 1.0f);
				mLightDir = glm::normalize(mLightDir);
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

	void render() override
	{
		auto mainWnd = cgb::context().main_window();
		auto inFlightIndex = mainWnd->in_flight_index_for_frame();
		
		auto cmdbfr = cgb::command_pool::create_single_use_command_buffer(cgb::context().graphics_queue());
		cmdbfr->begin_recording();
		cmdbfr->bind_pipeline(mPipeline);
		cmdbfr->bind_descriptors(mPipeline->layout(), { 
				cgb::binding(0, 0, mOffscreenImageViews[inFlightIndex]),
				cgb::binding(1, 0, mTLAS[inFlightIndex])
			}
		);

		// Set the push constants:
		auto pushConstantsForThisDrawCall = push_const_data { 
			mQuakeCam.view_matrix(),
			glm::vec4{mLightDir, 0.0f}
		};
		cmdbfr->handle().pushConstants(mPipeline->layout_handle(), vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR, 0, sizeof(pushConstantsForThisDrawCall), &pushConstantsForThisDrawCall);

		// TRACE. THA. RAYZ.
		cmdbfr->handle().traceRaysNV(
			mPipeline->shader_binding_table_handle(), 0,
			mPipeline->shader_binding_table_handle(), 2 * mPipeline->table_entry_size(), mPipeline->table_entry_size(),
			mPipeline->shader_binding_table_handle(), 1 * mPipeline->table_entry_size(), mPipeline->table_entry_size(),
			nullptr, 0, 0,
			mainWnd->swap_chain_extent().width, mainWnd->swap_chain_extent().height, 1,
			cgb::context().dynamic_dispatch());

		cmdbfr->end_recording();
		mainWnd->submit_for_backbuffer(std::move(cmdbfr));
		mainWnd->submit_for_backbuffer(mainWnd->copy_to_swapchain_image(mOffscreenImageViews[inFlightIndex]->get_image(), {}, true));
	}

private: // v== Member variables ==v
	std::chrono::high_resolution_clock::time_point mInitTime;

	// Multiple BLAS, concurrently used by all the (three?) TLASs:
	std::vector<cgb::bottom_level_acceleration_structure> mBLASs;
	// Geometry instance data which store the instance data per BLAS
	std::vector<cgb::geometry_instance> mGeometryInstances;
	// Multiple TLAS, one for each frame in flight:
	std::vector<cgb::top_level_acceleration_structure> mTLAS;

	std::vector<cgb::image_view> mOffscreenImageViews;

	glm::vec3 mLightDir = {0.0f, -1.0f, 0.0f};
	
	cgb::ray_tracing_pipeline mPipeline;
	cgb::quake_camera mQuakeCam;

}; // ray_tracing_basic_usage_app

int main() // <== Starting point ==
{
	try {
		// What's the name of our application
		cgb::settings::gApplicationName = "cg_base::ray_tracing_basic_usage";
		cgb::settings::gQueueSelectionPreference = cgb::device_queue_selection_strategy::prefer_everything_on_single_queue;
		cgb::settings::gRequiredDeviceExtensions.push_back(VK_KHR_RAY_TRACING_EXTENSION_NAME);
		cgb::settings::gRequiredDeviceExtensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
		cgb::settings::gRequiredDeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
		cgb::settings::gRequiredDeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		cgb::settings::gRequiredDeviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
		cgb::settings::gRequiredDeviceExtensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
		cgb::settings::gLoadImagesInSrgbFormatByDefault = true;

		// Create a window and open it
		auto mainWnd = cgb::context().create_window("cg_base: Real-Time Ray Tracing - Basic Usage Example");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->set_presentaton_mode(cgb::presentation_mode::fifo);
		mainWnd->open(); 

		// Create an instance of our main cgb::element which contains all the functionality:
		auto app = ray_tracing_basic_usage_app();
		// Create another element for drawing the UI with ImGui
		auto ui = cgb::imgui_manager();

		auto rtBasic = cgb::setup(app, ui);
		rtBasic.start();
	}
	catch (cgb::logic_error&) {}
	catch (cgb::runtime_error&) {}
}


