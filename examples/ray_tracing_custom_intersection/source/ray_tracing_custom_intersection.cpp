#include <cg_base.hpp>

class ray_tracing_custom_intersection_app : public cgb::cg_element
{
	struct transformation_matrices {
		glm::mat4 mViewMatrix;
	};

	// Define a struct for our vertex input data:
	struct Vertex {
	    glm::vec3 pos;
	    glm::vec3 color;
	};

	// Vertex data for drawing a pyramid:
	const std::vector<Vertex> mVertexData = {
		// pyramid front
		{{ 0.0f, -0.5f, 0.5f},  {1.0f, 0.0f, 0.0f}},
		{{ 0.3f,  0.5f, 0.2f},  {0.5f, 0.5f, 0.5f}},
		{{-0.3f,  0.5f, 0.2f},  {0.5f, 0.5f, 0.5f}},
		// pyramid right
		{{ 0.0f, -0.5f, 0.5f},  {1.0f, 0.0f, 0.0f}},
		{{ 0.3f,  0.5f, 0.8f},  {0.6f, 0.6f, 0.6f}},
		{{ 0.3f,  0.5f, 0.2f},  {0.6f, 0.6f, 0.6f}},
		// pyramid back
		{{ 0.0f, -0.5f, 0.5f},  {1.0f, 0.0f, 0.0f}},
		{{-0.3f,  0.5f, 0.8f},  {0.5f, 0.5f, 0.5f}},
		{{ 0.3f,  0.5f, 0.8f},  {0.5f, 0.5f, 0.5f}},
		// pyramid left
		{{ 0.0f, -0.5f, 0.5f},  {1.0f, 0.0f, 0.0f}},
		{{-0.3f,  0.5f, 0.2f},  {0.4f, 0.4f, 0.4f}},
		{{-0.3f,  0.5f, 0.8f},  {0.4f, 0.4f, 0.4f}},
	};

	// Indices for the faces (triangles) of the pyramid:
	const std::vector<uint16_t> mIndices = {
		 0, 1, 2,  3, 4, 5,  6, 7, 8,  9, 10, 11
	};

public: // v== cgb::cg_element overrides which will be invoked by the framework ==v

	void initialize() override
	{
		mInitTime = std::chrono::high_resolution_clock::now();
		
		// Add a pyramid:
		auto pyrVert = cgb::create_and_fill(
			cgb::vertex_buffer_meta::create_from_data(mVertexData).describe_member(&Vertex::pos, 0, cgb::content_description::position),
			cgb::memory_usage::host_coherent,
			mVertexData.data()
		);
		auto pyrIndex = cgb::create_and_fill(
			cgb::index_buffer_meta::create_from_data(mIndices),	
			cgb::memory_usage::host_coherent,
			mIndices.data()
		);
		auto pyrBlas = cgb::bottom_level_acceleration_structure_t::create(std::move(pyrVert), std::move(pyrIndex));
		pyrBlas.enable_shared_ownership();
		mBLASs.push_back(pyrBlas);
		mBLASs.back()->build();

		mGeometryInstances.push_back(
			cgb::geometry_instance::create(pyrBlas)
		);
		
		std::vector<cgb::aabb> aabbs = {
			{ { -0.5f, -0.5f, -0.5f }, {  0.5f,  0.5f,  0.5f } },
			{ {  1.0f,  1.0f,  1.0f }, {  3.0f,  3.0f,  3.0f } },
			{ { -3.0f, -2.0f, -1.0f }, { -2.0f, -1.0f,  0.0f } },
		};
		auto blas = cgb::bottom_level_acceleration_structure_t::create(aabbs, true);
		// Enable shared ownership because we'll have one TLAS per frame in flight, each one referencing the SAME BLASs
		// (But that also means that we may not modify the BLASs. They must stay the same, otherwise concurrent access will fail.)
		blas.enable_shared_ownership();
		mBLASs.push_back(blas); // No need to move, because a BLAS is now represented by a shared pointer internally. We could, though.
		mBLASs.back()->build();

		mGeometryInstances.push_back(
			cgb::geometry_instance::create(blas)
		);

		auto fif = cgb::context().main_window()->number_of_in_flight_frames();
		for (decltype(fif) i = 0; i < fif; ++i) {
			// Each TLAS owns every BLAS (this will only work, if the BLASs themselves stay constant, i.e. read access
			auto tlas = cgb::top_level_acceleration_structure_t::create(mGeometryInstances.size());
			// Build the TLAS, ...
			tlas->build(mGeometryInstances);
			mTLAS.push_back(std::move(tlas));
		}
		
		// Create offscreen image views to ray-trace into, one for each frame in flight:
		const auto wdth = cgb::context().main_window()->resolution().x;
		const auto hght = cgb::context().main_window()->resolution().y;
		const auto frmt = cgb::image_format::from_window_color_buffer(cgb::context().main_window());
		cgb::invoke_for_all_in_flight_frames(cgb::context().main_window(), [&](auto inFlightIndex){
			mOffscreenImageViews.emplace_back(
				cgb::image_view_t::create(
					cgb::image_t::create(wdth, hght, frmt, false, 1, cgb::memory_usage::device, cgb::image_usage::versatile_image)
				)
			);
			mOffscreenImageViews.back()->get_image().transition_to_layout({}, cgb::sync::with_barriers_on_current_frame());
			assert((mOffscreenImageViews.back()->config().subresourceRange.aspectMask & vk::ImageAspectFlagBits::eColor) == vk::ImageAspectFlagBits::eColor);
		});

		// Create our ray tracing pipeline with the required configuration:
		mPipeline = cgb::ray_tracing_pipeline_for(
			cgb::define_shader_table(
				cgb::ray_generation_shader("shaders/rt_aabb.rgen"),
				cgb::procedural_hit_group::create_with_rint_and_rchit("shaders/rt_aabb.rint", "shaders/rt_aabb.rchit"),
				cgb::procedural_hit_group::create_with_rint_and_rchit("shaders/rt_aabb.rint", "shaders/rt_aabb_secondary.rchit"),
				cgb::miss_shader("shaders/rt_aabb.rmiss"),
				cgb::miss_shader("shaders/rt_aabb_secondary.rmiss")
			),
			cgb::max_recursion_depth::set_to_max(),
			// Define push constants and descriptor bindings:
			cgb::push_constant_binding_data { cgb::shader_type::ray_generation, 0, sizeof(transformation_matrices) },
			cgb::binding(0, 0, mOffscreenImageViews[0]), // Just take any, this is just to define the layout
			cgb::binding(1, 0, mTLAS[0])				 // Just take any, this is just to define the layout
		);

		// Add the camera to the composition (and let it handle the updates)
		mQuakeCam.set_translation({ 0.0f, 0.0f, 0.0f });
		mQuakeCam.set_perspective_projection(glm::radians(60.0f), cgb::context().main_window()->aspect_ratio(), 0.5f, 100.0f);
		//mQuakeCam.set_orthographic_projection(-5, 5, -5, 5, 0.5, 100);
		cgb::current_composition().add_element(mQuakeCam);
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

		// Arrow Keys || Page Up/Down Keys => Move the TLAS
		static int64_t updateUntilFrame = -1;
		if (cgb::input().key_down(cgb::key_code::left) || cgb::input().key_down(cgb::key_code::right) || cgb::input().key_down(cgb::key_code::page_down) || cgb::input().key_down(cgb::key_code::page_up) || cgb::input().key_down(cgb::key_code::up) || cgb::input().key_down(cgb::key_code::down)) {
			// Make sure to update all of the in-flight TLASs, otherwise we'll get some geometry jumping:
			updateUntilFrame = cgb::context().main_window()->current_frame() + cgb::context().main_window()->number_of_in_flight_frames() - 1;
		}
		if (cgb::context().main_window()->current_frame() <= updateUntilFrame)
		{
			auto inFlightIndex = cgb::context().main_window()->in_flight_index_for_frame();
			
			auto x = (cgb::input().key_down(cgb::key_code::left)      ? -cgb::time().delta_time() : 0.0f)
					+(cgb::input().key_down(cgb::key_code::right)     ?  cgb::time().delta_time() : 0.0f);
			auto y = (cgb::input().key_down(cgb::key_code::page_down) ? -cgb::time().delta_time() : 0.0f)
					+(cgb::input().key_down(cgb::key_code::page_up)   ?  cgb::time().delta_time() : 0.0f);
			auto z = (cgb::input().key_down(cgb::key_code::up)        ? -cgb::time().delta_time() : 0.0f)
					+(cgb::input().key_down(cgb::key_code::down)      ?  cgb::time().delta_time() : 0.0f);
			auto speed = 1000.0f;
			
			// Change the position of one of the current TLASs BLAS, and update-build the TLAS.
			// The changes do not affect the BLAS, only the instance-data that the TLAS stores to each one of the BLAS.
			//
			// 1. Change every other instance:
			bool evenOdd = true;
			for (auto& geomInst : mGeometryInstances) {
				evenOdd = !evenOdd;
				if (evenOdd) { continue; }
				geomInst.set_transform(glm::translate(geomInst.mTransform, glm::vec3{x, y, z} * speed));
			}
			//
			// 2. Update the TLAS for the current inFlightIndex, copying the changed BLAS-data into an internal buffer:
			mTLAS[inFlightIndex]->update(mGeometryInstances, cgb::sync::with_barriers_on_current_frame(
				{}, // Nothing to wait for
				[](cgb::command_buffer_t& commandBuffer, cgb::pipeline_stage srcStage, std::optional<cgb::write_memory_access> srcAccess){
					// We want this update to be as efficient/as tight as possible
					commandBuffer.establish_global_memory_barrier(
						srcStage, cgb::pipeline_stage::ray_tracing_shaders, // => ray tracing shaders must wait on the building of the acceleration structure
						srcAccess, cgb::memory_access::acceleration_structure_read_access // TLAS-update's memory must be made visible to ray tracing shader's caches (so they can read from)
					);
				}
			));
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
		if (cgb::input().key_pressed(cgb::key_code::tab)) {
			if (mQuakeCam.is_enabled()) {
				mQuakeCam.disable();
			}
			else {
				mQuakeCam.enable();
			}
		}
	}

	void render() override
	{
		auto inFlightIndex = cgb::context().main_window()->in_flight_index_for_frame();
		
		auto cmdbfr = cgb::context().graphics_queue().create_single_use_command_buffer();
		cmdbfr->begin_recording();
		cmdbfr->bind_pipeline(mPipeline);
		cmdbfr->bind_descriptors(mPipeline->layout(), {
				cgb::binding(0, 0, mOffscreenImageViews[inFlightIndex]),
				cgb::binding(1, 0, mTLAS[inFlightIndex])
			}
		);

		// Set the push constants:
		auto pushConstantsForThisDrawCall = transformation_matrices { 
			mQuakeCam.view_matrix()
		};
		cmdbfr->handle().pushConstants(mPipeline->layout_handle(), vk::ShaderStageFlagBits::eRaygenNV, 0, sizeof(pushConstantsForThisDrawCall), &pushConstantsForThisDrawCall);

		// TRACE. THA. RAYZ.
		cmdbfr->handle().traceRaysNV(
			mPipeline->shader_binding_table_handle(), 0,
			mPipeline->shader_binding_table_handle(), 3 * mPipeline->table_entry_size(), mPipeline->table_entry_size(),
			mPipeline->shader_binding_table_handle(), 1 * mPipeline->table_entry_size(), mPipeline->table_entry_size(),
			nullptr, 0, 0,
			cgb::context().main_window()->swap_chain_extent().width, cgb::context().main_window()->swap_chain_extent().height, 1,
			cgb::context().dynamic_dispatch());

		cmdbfr->end_recording();
		submit_command_buffer_ownership(std::move(cmdbfr));
		present_image(mOffscreenImageViews[inFlightIndex]->get_image());
	}

	void finalize() override
	{
		cgb::context().logical_device().waitIdle();
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

	cgb::ray_tracing_pipeline mPipeline;
	cgb::quake_camera mQuakeCam;

}; // real_time_ray_tracing_app

int main() // <== Starting point ==
{
	try {
		// What's the name of our application
		cgb::settings::gApplicationName = "cg_base::real_time_ray_tracing";
		cgb::settings::gQueueSelectionPreference = cgb::device_queue_selection_strategy::prefer_everything_on_single_queue;
		cgb::settings::gRequiredDeviceExtensions.push_back(VK_NV_RAY_TRACING_EXTENSION_NAME);
		cgb::settings::gRequiredDeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
		cgb::settings::gLoadImagesInSrgbFormatByDefault = true;

		// Create a window and open it
		auto mainWnd = cgb::context().create_window("cg_base: Real-Time Ray Tracing - Custom Intersection Example");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->set_presentaton_mode(cgb::presentation_mode::vsync);
		mainWnd->set_additional_back_buffer_attachments({ cgb::attachment::create_depth(cgb::image_format::default_depth_format()) });
		mainWnd->open(); 

		// Create an instance of ray_tracing_custom_intersection_app which, in this case,
		// contains the entire functionality of our application. 
		auto element = ray_tracing_custom_intersection_app();

		// Create a composition of:
		//  - a timer
		//  - an executor
		//  - a behavior
		// ...
		auto hello = cgb::composition<cgb::varying_update_timer, cgb::sequential_executor>({
				&element
			});

		// ... and start that composition!
		hello.start();
	}
	catch (std::runtime_error& re)
	{
		LOG_ERROR_EM(re.what());
		//throw re;
	}
}


