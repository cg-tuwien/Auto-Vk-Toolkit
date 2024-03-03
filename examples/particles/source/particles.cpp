
#include "imgui.h"
#include "imgui_impl_vulkan.h"

#include "configure_and_compose.hpp"
#include "imgui_manager.hpp"
#include "invokee.hpp"
#include "sequential_invoker.hpp"

#include <thread>
#include <atomic>
#include <chrono>

constexpr static auto kConcurrentFrames = 3u;
constexpr uint64_t kParticleBufferSize = 3;

struct Particle {
	alignas(16) glm::vec3 pos;
	alignas(16) glm::vec3 vel;
	alignas(4) float size;
	alignas(4) float age;
};

struct AACube {
	alignas(16) glm::vec3 min;
	alignas(16) glm::vec3 max;
};

struct Sphere {
	alignas(16) glm::vec3 origin;
	alignas(4) float radius;
};

template<size_t COUNT>
struct Colliders {
	alignas(4) glm::uint cubeCount;
	alignas(4) glm::uint sphereCount;
	alignas(16) std::array<AACube, COUNT> cubes;
	alignas(16) std::array<Sphere, COUNT> spheres;
};

struct ParticleSystem{
	alignas(16) glm::vec3 origin;
	alignas(4) float spawnRadius;
	alignas(4) float spawnRate;
	alignas(16) glm::vec3 initialVelocity;
	alignas(16) glm::vec3 gravity;
	alignas(4) float particleSize;
	alignas(4) float particleLifetime;
};

template<size_t COUNT = 10>
struct Metadata {
	alignas(8) uint64_t runTime;
	alignas(8) uint64_t deltaTime;
	//alignas(16) ParticleSystem systemProperties;
	//alignas(16) Colliders<COUNT> colliders;
};

class draw_particle_system_app : public avk::invokee
{
public: // v== avk::invokee overrides which will be invoked by the framework ==v
	draw_particle_system_app(avk::queue& aRenderQueue, avk::queue& aParticleQueue) : mRenderQueue{ &aRenderQueue }, mParticleQueue{ &aParticleQueue }
	{}

	void initialize() override
	{
		// Print some information about the available memory on the selected physical device:
		avk::context().print_available_memory_types();

		mPhysicsRefreshRate.store(60);

		mRuntimeSemaphore = avk::context().create_timeline_semaphore(0);
		mParticleSnapshotSemaphore = avk::context().create_timeline_semaphore(0);

		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = avk::context().create_descriptor_cache();

		mParticleComputeBuffer = avk::context().create_buffer(
			avk::memory_usage::device, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc,
			avk::storage_buffer_meta::create_from_size(sizeof(Particle) * kParticleBufferSize)
		);

		for(auto &intermediateBuffer : mParticleIntermediateBuffers) {
			intermediateBuffer = avk::context().create_buffer(
				avk::memory_usage::device, vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
				avk::uniform_buffer_meta::create_from_size(sizeof(Particle) * kParticleBufferSize)
			);
		}

		for(size_t i = 0; i < kConcurrentFrames; i++) {
			mParticleVertexBuffers.push_back(
				avk::context().create_buffer(
					avk::memory_usage::device, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
					avk::uniform_buffer_meta::create_from_size(sizeof(Particle) * kParticleBufferSize))
			);
		}

		mMetadata = {0, 0};
		mMetadataBuffer = avk::context().create_buffer(avk::memory_usage::host_visible, {}, avk::uniform_buffer_meta::create_from_data(mMetadata));

		// Create a particle pipeline:
		mParticlePipeline = avk::context().create_compute_pipeline_for(
			avk::compute_shader("shaders/particles.comp"),
			avk::descriptor_binding(0, 0, mParticleComputeBuffer->as_storage_buffer()),  // add a descriptor for the particle buffer
			avk::descriptor_binding(0, 1, mMetadataBuffer->as_uniform_buffer())			// add metadata buffer
		);
		
		// Create a graphics pipeline:
		mRenderPipeline = avk::context().create_graphics_pipeline_for(
			avk::vertex_shader("shaders/particle.vert"),
			avk::fragment_shader("shaders/particle.frag"),
			avk::cfg::front_face::define_front_faces_to_be_clockwise(),
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)),
			avk::descriptor_binding(0,0, mParticleIntermediateBuffers[0]->as_uniform_buffer()),
			// Just use the main window's renderpass for this pipeline:
			avk::context().main_window()->get_renderpass()
		);

		// setup other threads
		mEpoch = clock::now();

		mComputeShaderDispatcher = std::thread(&draw_particle_system_app::physicsUpdate, this);


		// We want to use an updater => gotta create one:
		mUpdater.emplace();
		mUpdater->on(
			avk::swapchain_resized_event(avk::context().main_window()),       // In the case of window resizes,
			avk::shader_files_changed_event(mRenderPipeline.as_reference()),  // or in the case of changes to the shader files (hot reloading), ...
			avk::shader_files_changed_event(mParticlePipeline.as_reference()) // or in the case of changes to the shader files (hot reloading), ...
		)
		.update(mRenderPipeline, mParticlePipeline); // ... it will recreate the pipelines.

		auto imguiManager = avk::current_composition()->element_by_type<avk::imgui_manager>();
		if(nullptr != imguiManager) {
			imguiManager->add_callback([this](){	
				bool isEnabled = this->is_enabled();
				int currentPhysicsRefreshRate = mPhysicsRefreshRate.load();
				int physicsRefreshRate = currentPhysicsRefreshRate;

		        ImGui::Begin("Particles & Timeline Semaphores");
				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
				ImGui::Checkbox("Enable/Disable invokee", &isEnabled);				
				if (isEnabled != this->is_enabled())
				{					
					if (!isEnabled) this->disable();
					else this->enable();					
				}
				ImGui::SliderInt("Physics Refresh Rate (Hz)", &physicsRefreshRate, 1, 144);
				if(currentPhysicsRefreshRate != physicsRefreshRate) {
					mPhysicsRefreshRate.store(physicsRefreshRate);
				}

				static std::vector<float> values;
				values.push_back(1000.0f / ImGui::GetIO().Framerate);
		        if (values.size() > 90) {
			        values.erase(values.begin());
		        }
	            ImGui::PlotLines("ms/frame", values.data(), static_cast<int>(values.size()), 0, nullptr, 0.0f, FLT_MAX, ImVec2(0.0f, 100.0f));
		        ImGui::End();
			});
		}
	}

	void finalize() override {
		mStopped.store(true);
		if(mComputeShaderDispatcher.joinable()) {
			mComputeShaderDispatcher.join();
		}
	}

	void physicsUpdate() {
		uint64_t snapshotId = 1;
		while(!mStopped.load()) {
			mMetadata.deltaTime = (uint64_t)1e+6 / mPhysicsRefreshRate.load();
			int bufferId = snapshotId % mParticleIntermediateBuffers.size();



			auto& commandPool = avk::context().get_command_pool_for_single_use_command_buffers(*mParticleQueue);
			auto cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

			avk::context().record({
				mMetadataBuffer->fill(&mMetadata, 0),

				avk::sync::buffer_memory_barrier(
					mMetadataBuffer.as_reference(),
					avk::stage::host >> avk::stage::compute_shader,
					avk::access::host_write >> avk::access::uniform_read
				),

				avk::command::bind_pipeline(mParticlePipeline.as_reference()),
				avk::command::bind_descriptors(mParticlePipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
					avk::descriptor_binding(0,0, mParticleComputeBuffer->as_storage_buffer()),
					avk::descriptor_binding(0,1, mMetadataBuffer->as_uniform_buffer())
				})),
				avk::command::dispatch(1,1,1),

				avk::sync::buffer_memory_barrier(
					mParticleComputeBuffer.as_reference(),
					avk::stage::compute_shader >> avk::stage::all_transfer,
					avk::access::shader_storage_write >> avk::access::transfer_read
				),

				avk::copy_buffer_to_another(mParticleComputeBuffer, mParticleIntermediateBuffers[bufferId]),
								  })
				.into_command_buffer(cmdBfr)
				.then_submit_to(*mParticleQueue)
				.waiting_for(mParticleSnapshotSemaphore >= snapshotId - 1 >> avk::stage::all_commands)
				.signaling_upon_completion(avk::stage::all_transfer >> mParticleSnapshotSemaphore = snapshotId)
				.submit();

			mParticleSnapshotSemaphore->cleanup_expired_resources();
			mParticleSnapshotSemaphore->handle_lifetime_of(std::move(cmdBfr), snapshotId);
			mParticleSnapshotSemaphore->wait_until_signaled(snapshotId);

			uint64_t time = runtime();
			uint64_t waitTime = time > mMetadata.runTime ? 0 : mMetadata.runTime - time;
			std::this_thread::sleep_for(std::chrono::microseconds(waitTime));
			mMetadata.runTime += mMetadata.deltaTime;
			snapshotId++;
		}

		// let cmdbuffers execute before exiting the thread
		mParticleSnapshotSemaphore->wait_until_signaled(snapshotId - 1, 1e+9);
	}

	void update() override
	{
		// On C pressed,
		if (avk::input().key_pressed(avk::key_code::c)) {
			// center the cursor:
			auto resolution = avk::context().main_window()->resolution();
			avk::context().main_window()->set_cursor_pos({ resolution[0] / 2.0, resolution[1] / 2.0 });
		}

		// On Esc pressed,
		if (avk::input().key_pressed(avk::key_code::escape) || avk::context().main_window()->should_be_closed()) {
			// stop the current composition:
			avk::current_composition()->stop();
		}
	}

	/**	Render callback which is invoked by the framework every frame after every update() callback has been invoked.
	 *
	 *	Important: We must establish a dependency to the "swapchain image available" condition, i.e., we must wait for the
	 *	           next swap chain image to become available before we may start to render into it.
	 *			   This dependency is expressed through a semaphore, and the framework demands us to use it via the function:
	 *			   context().main_window()->consume_current_image_available_semaphore() for the main_window (our only window).
	 *
	 *			   More background information: At one point, we also must tell the presentation engine when we are done
	 *			   with rendering by the means of a semaphore. Actually, we would have to use the framework function:
	 *			   mainWnd->add_present_dependency_for_current_frame() for that purpose, but we don't have to do it in our case
	 *			   since we are rendering a GUI. imgui_manager will add a semaphore as dependency for the presentation engine.
	 */
	void render() override
	{
		auto mainWnd = avk::context().main_window();

		// The main window's swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();

		// Get a command pool to allocate command buffers from:
		auto& commandPool = avk::context().get_command_pool_for_single_use_command_buffers(*mRenderQueue);

		// Create a command buffer and render into the *current* swap chain image:
		auto cmdBfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		uint64_t snapshotId = mParticleSnapshotSemaphore->query_current_value();
		size_t bufferId = snapshotId % mParticleIntermediateBuffers.size();

		avk::context().record({
			/*avk::copy_buffer_to_another(mParticleComputeBuffer, mParticleIntermediateBuffers[bufferId]),

			avk::sync::buffer_memory_barrier(
				mParticleIntermediateBuffers[bufferId].as_reference(),
				avk::stage::all_transfer >> avk::stage::vertex_shader,
				avk::access::transfer_write >> avk::access::uniform_read
			),*/

			// Begin and end one renderpass:
			avk::command::render_pass(mRenderPipeline->renderpass_reference(), avk::context().main_window()->current_backbuffer_reference(), {
					avk::command::bind_pipeline(mRenderPipeline.as_reference()),
					avk::command::bind_descriptors(mRenderPipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
						avk::descriptor_binding(0, 0, mParticleIntermediateBuffers[bufferId]->as_uniform_buffer())
					})),
					avk::command::draw(kParticleBufferSize * 6, kParticleBufferSize * 2, 0u, 0u)
				})
			})
			.into_command_buffer(cmdBfr)
			.then_submit_to(*mRenderQueue)
			.waiting_for(imageAvailableSemaphore >> avk::stage::color_attachment_output)
			.submit();

		// Use a convenience function of avk::window to take care of the command buffer's lifetime:
		// It will get deleted in the future after #concurrent-frames have passed by.
		avk::context().main_window()->handle_lifetime(std::move(cmdBfr));
	}

private: // v== Member variables ==v
	using clock = std::chrono::steady_clock;
	using timepoint = clock::time_point;

	avk::queue* mRenderQueue;
	avk::queue* mParticleQueue;
	avk::buffer mParticleComputeBuffer;
	std::array<avk::buffer, 2> mParticleIntermediateBuffers;
	std::vector<avk::buffer> mParticleVertexBuffers;
	avk::buffer mMetadataBuffer;
	Metadata<10> mMetadata;
	avk::graphics_pipeline mRenderPipeline;
	avk::compute_pipeline mParticlePipeline;
	avk::descriptor_cache mDescriptorCache;

	avk::semaphore mRuntimeSemaphore;
	avk::semaphore mParticleSnapshotSemaphore;

	std::atomic_bool mStopped { false };
	std::atomic_uint mPhysicsRefreshRate;
	std::thread mComputeShaderDispatcher;
	timepoint mEpoch;

	uint64_t runtime() {
		using namespace std::chrono;
		return duration_cast<microseconds>(clock::now() - mEpoch).count();
	}

}; // draw_particle_system_app

int main() // <== Starting point ==
{
	int result = EXIT_FAILURE;
	try {
		// Create a window and open it
		auto mainWnd = avk::context().create_window("Particles & Timeline Semaphores");
		mainWnd->set_resolution({ 1920, 1080 });
		mainWnd->enable_resizing(true);
		mainWnd->set_presentaton_mode(avk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(kConcurrentFrames);
		mainWnd->set_number_of_presentable_images(3u);
		mainWnd->open();

		auto& renderQueue = avk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->set_queue_family_ownership(renderQueue.family_index());
		mainWnd->set_present_queue(renderQueue);
		
		auto& particleQueue = avk::context().create_queue(vk::QueueFlagBits::eCompute, avk::queue_selection_preference::specialized_queue, mainWnd);
		mainWnd->set_queue_family_ownership(particleQueue.family_index());
		
		// Create an instance of our main "invokee" which contains all the functionality:
		auto app = draw_particle_system_app(renderQueue, particleQueue);
		// Create another invokee for drawing the UI with ImGui
		auto ui = avk::imgui_manager(renderQueue);

		// Compile all the configuration parameters and the invokees into a "composition":
		auto composition = configure_and_compose(
			avk::application_name("Particles & Timeline Semaphores!"),
			[](avk::validation_layers& config) {
				config.enable_feature(vk::ValidationFeatureEnableEXT::eSynchronizationValidation);
			},
			[](vk::PhysicalDeviceFeatures& features) {
				features.setShaderInt64(VK_TRUE);
			},
			[](vk::PhysicalDeviceVulkan12Features& features) {
				features.setTimelineSemaphore(VK_TRUE);
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
