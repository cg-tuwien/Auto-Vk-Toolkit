#include <gvk.hpp>
#include <imgui.h>
#include "animated_model.hpp"

class model_loader_app : public gvk::invokee {

	struct transformation_matrix {
		glm::mat4 mModelMatrix;
		glm::vec4 mCamPos;
		int mMaterialIndex;
		int mSkinningMode;
	};

public:// v== avk::invokee overrides which will be invoked by the framework ==v
	model_loader_app(avk::queue &aQueue) : mQueue{&aQueue}, mScale{1.0f, 1.0f, 1.0f} {
	}

	void initialize() override {
		mInitTime = std::chrono::high_resolution_clock::now();

		// Create a descriptor cache that helps us to conveniently create descriptor
		// sets:
		mDescriptorCache = gvk::context().create_descriptor_cache();

		// setup camera buffer before initializing models
		mViewProjBuffer = gvk::context().create_buffer(
			avk::memory_usage::host_visible, {}, avk::uniform_buffer_meta::create_from_data(glm::mat4()));

		// Load a model from file:
		load_model_once_for_each_skinning_mode("skinning_test_tube_animation.fbx");

		// Create our rasterization graphics pipeline with the required
		// configuration:
		setup_pipelines();

		// Add the camera to the composition (and let it handle the updates)
		mQuakeCam.set_translation({0.0f, 1.5f, 4.0f});
		mQuakeCam.look_at(glm::vec3(0.0f, 1.5f, 0.0f));
		mQuakeCam.set_perspective_projection(
			glm::radians(60.0f), gvk::context().main_window()->aspect_ratio(), 0.3f, 1000.0f
		);
		gvk::current_composition()->add_element(mQuakeCam);
	}

	void load_model_once_for_each_skinning_mode (
		const std::string &filename,
		float global_z_offset = 0.0f,
		float position_x_offset_between_each = 3.0f
	) {
		auto& lbs_model = mAnimatedModels.emplace_back();
		lbs_model.initialize(
			"assets/" + filename,
			filename + "_linear_blend_skinning",
			glm::translate(glm::vec3(-position_x_offset_between_each, 0.0f, global_z_offset))
				* glm::scale(glm::vec3(1.0f, 1.0f, 1.0f))
				* glm::rotate(-glm::pi<float>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f)),
			false,
			100,
			animated_model::skinning_mode::linear_blend_skinning,
			0
		);

		auto& dqs_model = mAnimatedModels.emplace_back();
		dqs_model.initialize(
			"assets/" + filename,
			filename + "_dual_quaternion_skinning",
			glm::translate(glm::vec3(0.0f, 0.0f, 0.0f))
				* glm::scale(glm::vec3(1.0f, 1.0f, 1.0f))
				* glm::rotate(-glm::pi<float>() / 2.0f, glm::vec3(1.0f, 0.0f, global_z_offset)),
			false,
			100,
			animated_model::skinning_mode::dual_quaternion_skinning,
			0
		);

		auto& cors_model = mAnimatedModels.emplace_back();
		cors_model.initialize(
			"assets/" + filename,
			filename + "_optimized_centers_of_rotation_skinning",
			glm::translate(glm::vec3(position_x_offset_between_each, 0.0f, global_z_offset))
				* glm::scale(glm::vec3(1.0f, 1.0f, 1.0f))
				* glm::rotate(-glm::pi<float>() / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f)),
			false,
			100,
			animated_model::skinning_mode::optimized_centers_of_rotation_skinning,
			0
		);
	}

	void setup_pipelines() {
		for (auto &model: mAnimatedModels) {
			auto &pipeline = mPipelines.emplace_back();
			pipeline = gvk::context().create_graphics_pipeline_for(
				// Specify which shaders the pipeline consists of:
				avk::vertex_shader("shaders/skinning.vert"),
				avk::fragment_shader("shaders/skinning.frag"),
				// The next 3 lines define the format and location of the vertex shader
				// inputs: (The dummy values (like glm::vec3) tell the pipeline the
				// format of the respective input)
				avk::from_buffer_binding(0)->stream_per_vertex<glm::vec3>()->to_location(0), // Position
				avk::from_buffer_binding(1)->stream_per_vertex<glm::vec2>()->to_location(1), // Tex Coords
				avk::from_buffer_binding(2)->stream_per_vertex<glm::vec3>()->to_location(2), // Normal
				avk::from_buffer_binding(3)->stream_per_vertex<glm::vec3>()->to_location(3), // Tangent
				avk::from_buffer_binding(4)->stream_per_vertex<glm::vec3>()->to_location(4), // Bitangent
				avk::from_buffer_binding(5)->stream_per_vertex<glm::vec4>()->to_location(5), // Bone Weights
				avk::from_buffer_binding(6)->stream_per_vertex<glm::uvec4>()->to_location(6), // Bone Indices
				avk::from_buffer_binding(7)->stream_per_vertex<glm::vec3>()->to_location(7), // Centers Of Rotation
				// Some further settings:
				avk::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
				avk::cfg::culling_mode::cull_back_faces,
				avk::cfg::viewport_depth_scissors_config::from_framebuffer(
					gvk::context().main_window()->backbuffer_at_index(0)),
				// We'll render to the back buffer, which has a color attachment always,
				// and in our case additionally a depth attachment, which has been
				// configured when creating the window. See main() function!
				avk::attachment::declare(
					gvk::format_from_window_color_buffer(gvk::context().main_window()),
					avk::on_load::clear,
					avk::color(0),
					avk::on_store::store
					),
				// But not in presentable format, because ImGui comes after
				avk::attachment::declare(
					gvk::format_from_window_depth_buffer(gvk::context().main_window()),
					avk::on_load::clear,
					avk::depth_stencil(),
					avk::on_store::dont_care
					),
				// The following define additional data which we'll pass to the
				// pipeline: We'll pass two matrices to our vertex shader via push
				// constants:
				avk::push_constant_binding_data{avk::shader_type::vertex, 0, sizeof(transformation_matrix)},
				avk::descriptor_binding(0, 0, mViewProjBuffer),
				avk::descriptor_binding(1, 0, model.get_bone_data().mBoneMatricesBuffer),
				avk::descriptor_binding(2, 0, model.get_bone_data().mDualQuaternionsBuffer),
				avk::descriptor_binding(3, 0, model.get_material_data().mImageSamplers),
				avk::descriptor_binding(4, 0, model.get_material_data().mMaterialsBuffer)
			);
		}
	}

	void render() override {
		// Update animation:
		for (auto& model : mAnimatedModels) {
			if (model.is_animated()) {
				model.update();
				model.update_bone_matrices();
			}
		}

		// Render:
		auto mainWnd = gvk::context().main_window();

		auto viewProjMat = mQuakeCam.projection_matrix() * mQuakeCam.view_matrix();
		mViewProjBuffer->fill(glm::value_ptr(viewProjMat), 0, avk::sync::not_required());

		auto &commandPool = gvk::context().get_command_pool_for_single_use_command_buffers(*mQueue);
		auto cmdbfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		cmdbfr->begin_recording();
		cmdbfr->begin_render_pass_for_framebuffer(
			mPipelines[0]->get_renderpass(), gvk::context().main_window()->current_backbuffer());

		for (int i = 0; i < mAnimatedModels.size(); i++) {
			auto &model = mAnimatedModels[i];
			auto &pipeline = mPipelines[i];

			cmdbfr->bind_pipeline(avk::const_referenced(pipeline));
			cmdbfr->bind_descriptors(pipeline->layout(), mDescriptorCache.get_or_create_descriptor_sets({
				avk::descriptor_binding(0, 0, mViewProjBuffer),
				avk::descriptor_binding(1, 0, model.get_bone_data().mBoneMatricesBuffer),
				avk::descriptor_binding(2, 0, model.get_bone_data().mDualQuaternionsBuffer),
				avk::descriptor_binding(3, 0, model.get_material_data().mImageSamplers),
				avk::descriptor_binding(4, 0, model.get_material_data().mMaterialsBuffer)
			}));
			for (auto& meshData : model.get_mesh_data()) {
				auto pushConst = transformation_matrix {
					// Set model matrix for this mesh:
					model.get_model_transform(),
					// Camera position
					glm::vec4(mQuakeCam.translation(), 1.0f),
					// Set material index for this mesh:
					meshData.mMaterialIndex,
					// Skinning mode (linear blend, dual quat or cors)
					static_cast<int>(model.get_skinning_mode())
				};
				cmdbfr->handle().pushConstants(
					pipeline->layout_handle(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(pushConst),
					&pushConst);

				cmdbfr->draw_indexed(
					avk::const_referenced(meshData.mIndexBuffer),
					avk::const_referenced(meshData.mPositionsBuffer),
					avk::const_referenced(meshData.mTexCoordsBuffer),
					avk::const_referenced(meshData.mNormalsBuffer),
					avk::const_referenced(meshData.mTangentsBuffer),
					avk::const_referenced(meshData.mBiTangentsBuffer),
					avk::const_referenced(meshData.mBoneWeightsBuffer),
					avk::const_referenced(meshData.mBoneIndexBuffer),
					avk::const_referenced(meshData.mCentersOfRotationBuffer));
			}
		}

		cmdbfr->end_render_pass();
		cmdbfr->end_recording();

		// The swap chain provides us with an "image available semaphore" for the
		// current frame. Only after the swapchain image has become available, we
		// may start rendering into it.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();

		// Submit the draw call and take care of the command buffer's lifetime:
		mQueue->submit(cmdbfr, imageAvailableSemaphore);
		mainWnd->handle_lifetime(avk::owned(cmdbfr));
	}

	void update() override {
		static int counter = 0;
		if (++counter == 4) {
			auto current = std::chrono::high_resolution_clock::now();
			auto time_span = current - mInitTime;
			auto int_min = std::chrono::duration_cast<std::chrono::minutes>(time_span).count();
			auto int_sec = std::chrono::duration_cast<std::chrono::seconds>(time_span).count();
			auto fp_ms = std::chrono::duration<double, std::milli>(time_span).count();
			printf(
				"Time from init to fourth frame: %d min, %lld sec %lf ms\n",
				int_min,
				int_sec - static_cast<decltype(int_sec)>(int_min) * 60,
				fp_ms - 1000.0 * int_sec
			);
		}

		if (gvk::input().key_pressed(gvk::key_code::c)) {
			// Center the cursor:
			auto resolution = gvk::context().main_window()->resolution();
			gvk::context().main_window()->set_cursor_pos({resolution[0] / 2.0, resolution[1] / 2.0});
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
			mQuakeCam.look_at(glm::vec3{0.0f, 0.0f, 0.0f});
		}

		if (gvk::input().key_pressed(gvk::key_code::f1)) {
			auto imguiManager = gvk::current_composition()->element_by_type<gvk::imgui_manager>();
			if (mQuakeCam.is_enabled()) {
				mQuakeCam.disable();
				if (nullptr != imguiManager) {
					imguiManager->enable_user_interaction(true);
				}
			} else {
				mQuakeCam.enable();
				if (nullptr != imguiManager) {
					imguiManager->enable_user_interaction(false);
				}
			}
		}
	}

private: // v== Member variables ==v
	std::chrono::high_resolution_clock::time_point mInitTime;

	avk::queue *mQueue;
	avk::descriptor_cache mDescriptorCache;

	avk::buffer mViewProjBuffer;
	std::vector<avk::graphics_pipeline> mPipelines;

	std::vector<animated_model> mAnimatedModels;
	gvk::quake_camera mQuakeCam;

	glm::vec3 mScale;
}; // model_loader_app

int main() // <== Starting point ==
{
	try {
		// Create a window and open it
		auto mainWnd = gvk::context().create_window("Vertex Skinning");

		mainWnd->set_resolution({1600, 880});
		mainWnd->enable_resizing(true);
		mainWnd->set_additional_back_buffer_attachments(
			{
				avk::attachment::declare(
					vk::Format::eD32Sfloat, avk::on_load::clear, avk::depth_stencil(), avk::on_store::dont_care
				)
			}
		);
		mainWnd->set_presentaton_mode(gvk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto &singleQueue = gvk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->add_queue_family_ownership(singleQueue);
		mainWnd->set_present_queue(singleQueue);

		// Create an instance of our main avk::element which contains all the
		// functionality:
		auto app = model_loader_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = gvk::imgui_manager(singleQueue);

		// GO:
		gvk::start(
			gvk::required_device_extensions(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME),
			gvk::application_name("Gears-Vk + Auto-Vk Example: Model Loader"),
			mainWnd,
			app,
			ui
		);
	} catch (gvk::logic_error &) {
	} catch (gvk::runtime_error &) {
	} catch (avk::logic_error &) {
	} catch (avk::runtime_error &) {
	}
}