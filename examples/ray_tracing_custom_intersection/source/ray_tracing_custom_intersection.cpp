
#include "imgui.h"

#include "configure_and_compose.hpp"
#include "imgui_manager.hpp"
#include "invokee.hpp"
#include "quake_camera.hpp"
#include "sequential_invoker.hpp"
#include "vk_convenience_functions.hpp"
#include "conversion_utils.hpp"

class ray_tracing_custom_intersection_app : public avk::invokee
{
	struct transformation_matrices {
		glm::mat4 mCameraTransform;
	};

	// Define a struct for our vertex input data:
	struct Vertex {
	    glm::vec3 mPosition;
	    glm::vec3 mColor;
	};

public: // v== avk::invokee overrides which will be invoked by the framework ==v
	ray_tracing_custom_intersection_app(avk::queue& aQueue)
		: mQueue{ &aQueue }
	{}


	void set_bottom_level_aabb_data()
	{
		mAabbs[0] = VkAabbPositionsKHR{ /* min: */ -0.5f, -0.5f, -0.5f, /* max: */  0.5f,  0.5f,  0.5f };
		mAabbs[1] = VkAabbPositionsKHR{ /* min: */  1.0f,  1.0f,  1.0f, /* max: */  3.0f,  3.0f,  3.0f };
		mAabbs[2] = VkAabbPositionsKHR{ /* min: */ -3.0f, -2.0f, -1.0f, /* max: */ -2.0f, -1.0f,  0.0f };
	}

	void add_geometry_instances_for_bottom_level_acc_structure(size_t aIndex, const avk::bottom_level_acceleration_structure_t& aBlas, glm::vec3 aTranslation) {
		mTranslations[aIndex + 0] =  aTranslation;
		mTranslations[aIndex + 1] = -aTranslation;
		mGeometryInstances.emplace_back( avk::context().create_geometry_instance( aBlas ).set_transform_column_major(avk::to_array(glm::translate(glm::mat4{1.0f}, mTranslations[aIndex + 0]))) );
		mGeometryInstances.emplace_back( avk::context().create_geometry_instance( aBlas ).set_transform_column_major(avk::to_array(glm::translate(glm::mat4{1.0f}, mTranslations[aIndex + 1]))) );
	}

	void set_bottom_level_pyramid_data()
	{
		mPyramidVertices = {
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
		mPyramidIndices = {
			 0, 1, 2,  3, 4, 5,  6, 7, 8,  9, 10, 11
		};
	}

	void build_pyramid_buffers()
	{
		auto& vtxBfr = mPyramidVertexBuffers.emplace_back(avk::context().create_buffer(
			avk::memory_usage::host_coherent,
#if VK_HEADER_VERSION >= 162
#else
			vk::BufferUsageFlagBits::eRayTracingKHR |
#endif
			vk::BufferUsageFlagBits::eShaderDeviceAddressKHR,
			avk::vertex_buffer_meta::create_from_data(mPyramidVertices).describe_member(&Vertex::mPosition, avk::content_description::position),
			avk::read_only_input_to_acceleration_structure_builds_buffer_meta::create_from_data(mPyramidVertices)
		));
		auto emptyCmd = vtxBfr->fill(mPyramidVertices.data(), 0);
		
		auto& idxBfr = mPyramidIndexBuffers.emplace_back( avk::context().create_buffer(
			avk::memory_usage::host_coherent, 
#if VK_HEADER_VERSION >= 162
#else
			vk::BufferUsageFlagBits::eRayTracingKHR |
#endif
			vk::BufferUsageFlagBits::eShaderDeviceAddressKHR,
			avk::index_buffer_meta::create_from_data(mPyramidIndices),
			avk::read_only_input_to_acceleration_structure_builds_buffer_meta::create_from_data(mPyramidIndices)
		));
		auto emptyToo = idxBfr->fill(mPyramidIndices.data(), 0);
	}

	void initialize() override
	{
		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = avk::context().create_descriptor_cache();
		
		auto* mainWnd = avk::context().main_window();
		auto fif = static_cast<size_t>(mainWnd->number_of_frames_in_flight());
		for (size_t i = 0; i < fif; ++i) {
			// ------------ Add AABBs to bottom-level acceleration structures: --------------
			if (0 == i /* only once */ ) {
				set_bottom_level_aabb_data();
			}
			
			auto& bLast = mBLAS.emplace_back();
			auto& tLast = mTLAS.emplace_back();
			
			for (size_t j=0; j < 3; ++j) {
				bLast[j] = avk::context().create_bottom_level_acceleration_structure(
					{ avk::acceleration_structure_size_requirements::from_aabbs(1u) }, 
					true /* allow updates */
				);
				avk::context().record_and_submit_with_fence({ bLast[j]->build({ mAabbs[j] }) }, *mQueue)->wait_until_signalled();

				if (0 == i) {
					add_geometry_instances_for_bottom_level_acc_structure(2*j, bLast[j].as_reference(), glm::vec3{ 0.0f, 0.0f, -3.0f });
				}
			}

			// ------------ Add pyramid to bottom-level acceleration structures: -------------
			if (0 == i /* only once */ ) {
				// Create the data:
				set_bottom_level_pyramid_data();
			}
			// Create the buffers:
			build_pyramid_buffers();

			// Put data into an acceleration structure:
			const size_t pyrOffset = 3;
			for (size_t j=0; j < 2; ++j) {
				bLast[pyrOffset + j] = avk::context().create_bottom_level_acceleration_structure(
					{ avk::acceleration_structure_size_requirements::from_buffers( avk::vertex_index_buffer_pair{ mPyramidVertexBuffers[i], mPyramidIndexBuffers[i] } ) },
					true /* allow updates */
				);
				avk::context().record_and_submit_with_fence({ bLast[pyrOffset + j]->build({ avk::vertex_index_buffer_pair{ mPyramidVertexBuffers[i], mPyramidIndexBuffers[i] } }) }, *mQueue)->wait_until_signalled();
			}
			
			if (0 == i /* only once */ ) {
				add_geometry_instances_for_bottom_level_acc_structure(2*pyrOffset, bLast[pyrOffset].as_reference(), glm::vec3{ 0.0f, -3.0f, 0.0f });
			}

			// ----------- Build the top-level acceleration structure (for this frame in flight): -------------
			assert (8 == mGeometryInstances.size());
			tLast = avk::context().create_top_level_acceleration_structure(static_cast<uint32_t>(mGeometryInstances.size()));
			avk::context().record_and_submit_with_fence({ tLast->build(mGeometryInstances) }, *mQueue)->wait_until_signalled();
		}
		
		// Create offscreen image views to ray-trace into, one for each frame in flight:
		const auto wdth = mainWnd->resolution().x;
		const auto hght = mainWnd->resolution().y;
		const auto frmt = avk::format_from_window_color_buffer(mainWnd);
		for (decltype(fif) i = 0; i < fif; ++i) {
			mOffscreenImageViews.emplace_back(
				avk::context().create_image_view(
					avk::context().create_image(wdth, hght, frmt, 1, avk::memory_usage::device, avk::image_usage::general_storage_image)
				)
			);
			avk::context().record_and_submit_with_fence({
				avk::sync::image_memory_barrier(mOffscreenImageViews.back()->get_image(), avk::stage::none >> avk::stage::none, avk::access::none >> avk::access::none)
					.with_layout_transition(avk::layout::undefined >> avk::layout::general)
			}, *mQueue)->wait_until_signalled();
			assert((mOffscreenImageViews.back()->create_info().subresourceRange.aspectMask & vk::ImageAspectFlagBits::eColor) == vk::ImageAspectFlagBits::eColor);
		}

		// Create our ray tracing pipeline with the required configuration:
		mPipeline = avk::context().create_ray_tracing_pipeline_for(
			avk::define_shader_table(
				avk::ray_generation_shader("shaders/rt_aabb.rgen"),
				avk::procedural_hit_group::create_with_rint_and_rchit("shaders/rt_aabb.rint", "shaders/rt_aabb.rchit"),
				avk::procedural_hit_group::create_with_rint_and_rchit("shaders/rt_aabb.rint", "shaders/rt_aabb_secondary.rchit"),
				avk::miss_shader("shaders/rt_aabb.rmiss"),
				avk::miss_shader("shaders/rt_aabb_secondary.rmiss")
			),
			avk::context().get_max_ray_tracing_recursion_depth(),
			// Define push constants and descriptor bindings:
			avk::push_constant_binding_data { avk::shader_type::ray_generation, 0, sizeof(transformation_matrices) },
			avk::descriptor_binding(0, 0, mOffscreenImageViews[0]->as_storage_image(avk::layout::general)), // Just take any, this is just to define the layout
			avk::descriptor_binding(1, 0, mTLAS[0])                                                         // Just take any, this is just to define the layout
		);
		
		mPipeline->print_shader_binding_table_groups();

		// Add the camera to the composition (and let it handle the updates)
		mQuakeCam.set_translation({ 0.0f, 0.0f, 0.0f });
		mQuakeCam.set_perspective_projection(glm::radians(60.0f), mainWnd->aspect_ratio(), 0.5f, 100.0f);
		//mQuakeCam.set_orthographic_projection(-5, 5, -5, 5, 0.5, 100);
		avk::current_composition()->add_element(mQuakeCam);

		auto imguiManager = avk::current_composition()->element_by_type<avk::imgui_manager>();
		if(nullptr != imguiManager) {
			imguiManager->add_callback([this](){
		        ImGui::Begin("Info & Settings");
				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);

				static std::vector<float> accum; // accumulate (then average) 10 frames
				accum.push_back(ImGui::GetIO().Framerate);
				static std::vector<float> values;
				if (accum.size() == 10) {
					values.push_back(std::accumulate(std::begin(accum), std::end(accum), 0.0f) / 10.0f);
					accum.clear();
				}
		        if (values.size() > 90) { // Display up to 90(*10) history frames
			        values.erase(values.begin());
		        }
	            ImGui::PlotLines("FPS", values.data(), static_cast<int>(values.size()), 0, nullptr, 0.0f, 1500.0f, ImVec2(0.0f, 150.0f));
				
				ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), "[F1]: Toggle input-mode");
				ImGui::TextColored(ImVec4(0.f, .6f, .8f, 1.f), " (UI vs. scene navigation)");

				ImGui::Separator();
				ImGui::TextColored(ImVec4(0.f, 0.8f, 0.6f, 1.f), "Modify Bottom Level Acceleration Structures:");
				for (int i=0; i < mAabbs.size(); ++i) {
					ImGui::DragFloat3(std::format("AABB[{}].min", i).c_str(), *reinterpret_cast<float(*)[3]>(&mAabbs[i].minX), 0.01f);
					ImGui::DragFloat3(std::format("AABB[{}].max", i).c_str(), *reinterpret_cast<float(*)[3]>(&mAabbs[i].maxX), 0.01f);
				}
				ImGui::DragFloat3("Pyramid Spire", *reinterpret_cast<float(*)[3]>(&mPyramidVertices[0].mPosition), 0.01f);

				ImGui::Separator();
				ImGui::TextColored(ImVec4(0.8f, 0.0f, 0.6f, 1.f), "Modify Top Level Acceleration Structures:");
				for (int i=0; i < mTranslations.size(); ++i) {
					ImGui::DragFloat3(std::format("Instance[{}].translation", i).c_str(), *reinterpret_cast<float(*)[3]>(&mTranslations[i]), 0.01f);
				}

				ImGui::Separator();

				ImGui::Checkbox("Update Bottom Level Acceleration Structures", &mUpdateBlas);
				ImGui::Checkbox("Update Top Level Acceleration Structures", &mUpdateTlas);
				
				ImGui::End();
			});
		}
	}

	void update() override
	{
		auto* mainWnd = avk::context().main_window();

		static avk::window::frame_id_t updateBlasUntilFrame = -1;
		static avk::window::frame_id_t updateTlasUntilFrame = -1;
		if (mUpdateBlas) {
			updateBlasUntilFrame = mainWnd->current_frame() + mainWnd->number_of_frames_in_flight() - 1;
		}
		if (mUpdateTlas) {
			updateTlasUntilFrame = mainWnd->current_frame() + mainWnd->number_of_frames_in_flight() - 1;
		}
		
		if (mUpdateBlas || mainWnd->current_frame() <= updateBlasUntilFrame) {
			const auto inFlightIndex = mainWnd->in_flight_index_for_frame();
			for (size_t i=0; i < mBLAS[inFlightIndex].size(); ++i) {
				if (i < 3) { // 3 AABBs
					auto cmdBfr = avk::context().record_and_submit({
						mBLAS[inFlightIndex][i]->build({ mAabbs[i] }, {}),
						avk::sync::global_memory_barrier(
							avk::stage::acceleration_structure_build  >> avk::stage::ray_tracing_shader,
							avk::access::acceleration_structure_write >> avk::access::acceleration_structure_read
						)
					}, *mQueue);

					mainWnd->handle_lifetime(std::move(cmdBfr));
				}
				else { // 1 triangle mesh
					for (int i=3; i < mPyramidVertices.size(); i+=3) {
						mPyramidVertices[i].mPosition = mPyramidVertices[0].mPosition;
					}
					// Both of them are host coherent buffers => no need for barriers, or executing their commands:
					mPyramidVertexBuffers[inFlightIndex]->fill(mPyramidVertices.data(), 0);
					mPyramidIndexBuffers[inFlightIndex]->fill(mPyramidIndices.data(), 0);

					auto cmdBfr = avk::context().record_and_submit({
						mBLAS[inFlightIndex][i]->update({ avk::vertex_index_buffer_pair{ mPyramidVertexBuffers[inFlightIndex], mPyramidIndexBuffers[inFlightIndex] } }),
						avk::sync::global_memory_barrier(
							avk::stage::acceleration_structure_build  >> avk::stage::ray_tracing_shader,
							avk::access::acceleration_structure_write >> avk::access::acceleration_structure_read
						)
					}, *mQueue);

					mainWnd->handle_lifetime(std::move(cmdBfr));
				}
			}
		}

		if (mUpdateTlas || mainWnd->current_frame() <= updateTlasUntilFrame) {
			const auto inFlightIndex = mainWnd->in_flight_index_for_frame();
			for (size_t i=0; i < mGeometryInstances.size(); ++i) {
				mGeometryInstances[i].mTransform.matrix[0][3] = mTranslations[i][0];
				mGeometryInstances[i].mTransform.matrix[1][3] = mTranslations[i][1];
				mGeometryInstances[i].mTransform.matrix[2][3] = mTranslations[i][2];
			}

			auto cmdBfr = avk::context().record_and_submit({
				avk::sync::global_memory_barrier(
					avk::stage::acceleration_structure_build  >> avk::stage::acceleration_structure_build,
					avk::access::acceleration_structure_write >> avk::access::acceleration_structure_read
				),
				mTLAS[inFlightIndex]->update(mGeometryInstances, {}),
				avk::sync::global_memory_barrier(
					avk::stage::acceleration_structure_build >> avk::stage::ray_tracing_shader,
					avk::access::acceleration_structure_write >> avk::access::acceleration_structure_read
				)
			}, *mQueue);

			mainWnd->handle_lifetime(std::move(cmdBfr));
		}

		if (avk::input().key_pressed(avk::key_code::space)) {
			// Print the current camera position
			auto pos = mQuakeCam.translation();
			LOG_INFO(std::format("Current camera position: {}", avk::to_string(pos)));
		}
		if (avk::input().key_pressed(avk::key_code::escape) || avk::context().main_window()->should_be_closed()) {
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
		auto inFlightIndex = mainWnd->in_flight_index_for_frame();

		// Get a command pool to allocate command buffers from:
		auto& commandPool = avk::context().get_command_pool_for_single_use_command_buffers(*mQueue);

		// Create a command buffer and render into the *current* swap chain image:
		auto cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		
		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();

		avk::context().record({
				avk::command::bind_pipeline(mPipeline.as_reference()),
				avk::command::bind_descriptors(mPipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
					avk::descriptor_binding(0, 0, mOffscreenImageViews[inFlightIndex]->as_storage_image(avk::layout::general)),
					avk::descriptor_binding(1, 0, mTLAS[inFlightIndex])
				})),
				avk::command::push_constants(mPipeline->layout(), transformation_matrices { mQuakeCam.global_transformation_matrix() }),

				// Do it:
				avk::command::trace_rays(
					avk::for_each_pixel(mainWnd),
					mPipeline->shader_binding_table(),
					avk::using_raygen_group_at_index(0),
					avk::using_miss_group_at_index(0),
					avk::using_hit_group_at_index(0)
				),

				// Wait until ray tracing has completed, then transfer the result image into the swap chain image:
				avk::sync::image_memory_barrier(mOffscreenImageViews[inFlightIndex]->get_image(),
					avk::stage::ray_tracing_shader >> avk::stage::copy,
					avk::access::shader_write      >> avk::access::transfer_read
				).with_layout_transition(avk::layout::general >> avk::layout::transfer_src),
				avk::sync::image_memory_barrier(mainWnd->current_backbuffer_reference().image_at(0),
					avk::stage::none               >> avk::stage::copy,
					avk::access::none              >> avk::access::transfer_write
				).with_layout_transition(avk::layout::undefined >> avk::layout::transfer_dst),

				avk::copy_image_to_another(
					mOffscreenImageViews[inFlightIndex]->get_image(), avk::layout::transfer_src,
					mainWnd->current_backbuffer_reference().image_at(0), avk::layout::transfer_dst
				),

				avk::sync::image_memory_barrier(mOffscreenImageViews[inFlightIndex]->get_image(),
					avk::stage::copy               >> avk::stage::ray_tracing_shader,
					avk::access::transfer_read     >> avk::access::shader_write
				).with_layout_transition(avk::layout::transfer_src >> avk::layout::general),
				avk::sync::image_memory_barrier(mainWnd->current_backbuffer_reference().image_at(0), // Prepare for ImGui
					avk::stage::copy               >> avk::stage::color_attachment_output,
					avk::access::transfer_write    >> avk::access::color_attachment_write
				).with_layout_transition(avk::layout::transfer_dst >> avk::layout::color_attachment_optimal),
			})
			.into_command_buffer(cmdBfr)
			.then_submit_to(*mQueue)
			// Do not start to render before the image has become available:
			.waiting_for(imageAvailableSemaphore >> avk::stage::color_attachment_output)
			.submit();

		// Submit the draw call and take care of the command buffer's lifetime:
		mainWnd->handle_lifetime(std::move(cmdBfr));
	}

private: // v== Member variables ==v
	avk::queue* mQueue;
	avk::descriptor_cache mDescriptorCache;
	std::vector<avk::image_view> mOffscreenImageViews;
	avk::ray_tracing_pipeline mPipeline;
	avk::quake_camera mQuakeCam;

	// Bottom level acceleration structure data:
	std::array<VkAabbPositionsKHR, 3> mAabbs;
	std::vector<Vertex> mPyramidVertices;
	std::vector<uint16_t> mPyramidIndices;

	// Bottom level acceleration structures (3 AABBs + 1 pyramid) -- and all of that per frame in flight.
	std::vector<avk::buffer> mPyramidVertexBuffers;
	std::vector<avk::buffer> mPyramidIndexBuffers;
	std::vector<std::array<avk::bottom_level_acceleration_structure, 5>> mBLAS;

	// Geometry instance data per every bottom level acceleration structure (3 AABBs + 1 pyramid) x2 (i.e. 2 instances each)
	std::vector<avk::geometry_instance> mGeometryInstances;
	
	// Top level acceleration structure containing 4x2 geometry instances -- per frame in flight.
	std::vector<avk::top_level_acceleration_structure> mTLAS;

	// Settings from the UI:
	bool mUpdateBlas = false;
	bool mUpdateTlas = false;
	std::array<glm::vec3, 8> mTranslations; // Each AABB x2 + two pyramids
	
}; // ray_tracing_custom_intersection_app

int main() // <== Starting point ==
{
	int result = EXIT_FAILURE;
	try {
		// Create a window and open it
		auto mainWnd = avk::context().create_window("Real-Time Ray Tracing - Custom Intersection Example");
		mainWnd->set_resolution({ 1920, 1080 });
		mainWnd->set_presentaton_mode(avk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = avk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->set_queue_family_ownership(singleQueue.family_index());
		mainWnd->set_present_queue(singleQueue);
		
		// Create an instance of our main avk::element which contains all the functionality:
		auto app = ray_tracing_custom_intersection_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = avk::imgui_manager(singleQueue);

		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			avk::application_name("Auto-Vk-Toolkit Example: Real-Time Ray Tracing - Custom Intersection Example"),
#if VK_HEADER_VERSION >= 162
			avk::required_device_extensions()
				// We need several extensions for ray tracing:
				.add_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
				.add_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
				.add_extension(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME)
				.add_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)
				.add_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)
				.add_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)
				.add_extension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME),
			[](vk::PhysicalDeviceVulkan12Features& aVulkan12Featues) {
				aVulkan12Featues.setBufferDeviceAddress(VK_TRUE);
			},
			[](vk::PhysicalDeviceRayTracingPipelineFeaturesKHR& aRayTracingFeatures) {
				aRayTracingFeatures.setRayTracingPipeline(VK_TRUE);
			},
				[](vk::PhysicalDeviceAccelerationStructureFeaturesKHR& aAccelerationStructureFeatures) {
				aAccelerationStructureFeatures.setAccelerationStructure(VK_TRUE);
			},
#else 
			avk::required_device_extensions()
				.add_extension(VK_KHR_RAY_TRACING_EXTENSION_NAME)
				.add_extension(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME)
				.add_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)
				.add_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)
				.add_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)
				.add_extension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME),
			[](vk::PhysicalDeviceVulkan12Features& aVulkan12Featues) {
				aVulkan12Featues.setBufferDeviceAddress(VK_TRUE);
			},
			[](vk::PhysicalDeviceRayTracingFeaturesKHR& aRayTracingFeatures) {
				aRayTracingFeatures.setRayTracing(VK_TRUE);
			},
#endif
			mainWnd,
			app,
			ui
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
