
#include "imgui.h"
#include "imgui_impl_vulkan.h"

#include "configure_and_compose.hpp"
#include "imgui_manager.hpp"
#include "invokee.hpp"
#include "sequential_invoker.hpp"

constexpr static auto kConcurrentFrames = 3u;

struct Particle {
	alignas(16) glm::vec3 pos;
	alignas(16) glm::vec3 vel;
	alignas(4) float size;
	alignas(4) float age;
};

struct AACube {
	glm::vec3 min;
	glm::vec3 max;
};

struct Sphere {
	glm::vec3 origin;
	float radius;
};

template<size_t COUNT>
struct Colliders {
	glm::uint cubeCount;
	glm::uint sphereCount;
	std::array<AACube, COUNT> cubes;
	std::array<Sphere, COUNT> spheres;
};

struct ParticleSystem{
	glm::vec3 origin;
	float spawnRadius;
	float spawnRate;
	glm::vec3 initialVelocity;
	glm::vec3 gravity;
	float particleSize;
	float particleLifetime;
};

template<size_t COUNT = 10>
struct Metadata {
	glm::uint targetBuffer;
	float deltaTime;
	ParticleSystem systemProperties;
	Colliders<COUNT> colliders;
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

		constexpr uint64_t kBufferSize = 100000;

		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = avk::context().create_descriptor_cache();

		auto createParticleBuffer = [&]() {
			return avk::context().create_buffer(avk::memory_usage::device, {}, avk::storage_buffer_meta::create_from_size(sizeof(Particle) * kBufferSize));
		};

		mParticleComputeBuffer = createParticleBuffer();

		mParticleIntermediateBuffer = createParticleBuffer();

		for(size_t i = 0; i < kConcurrentFrames; i++) {
			mParticleVertexBuffers.push_back(createParticleBuffer());
		}

		mHostParticleBuffer = avk::context().create_buffer(avk::memory_usage::host_coherent, {}, avk::uniform_buffer_meta::create_from_size(sizeof(Particle) * 3));

		mMetadata = {};
		mMetadataBuffer = avk::context().create_buffer(avk::memory_usage::host_visible, {}, avk::uniform_buffer_meta::create_from_data(mMetadata));

		// Create a particle pipeline:
		mParticlePipeline = avk::context().create_compute_pipeline_for(
			avk::compute_shader("shaders/particles.comp"),
			avk::descriptor_binding(0,0, mParticleComputeBuffer->as_storage_buffer()),  // add a descriptor for the particle buffer
			avk::descriptor_binding(0,1, mMetadataBuffer->as_uniform_buffer())			// add metadata buffer
		);
		
		// Create a graphics pipeline:
		mRenderPipeline = avk::context().create_graphics_pipeline_for(
			avk::vertex_shader("shaders/particle.vert"),
			avk::fragment_shader("shaders/particle.frag"),
			avk::cfg::front_face::define_front_faces_to_be_clockwise(),
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(avk::context().main_window()->backbuffer_reference_at_index(0)),
			//avk::from_buffer_binding(0)->stream_per_vertex(&Particle::pos)->to_location(0),
			//avk::from_buffer_binding(0)->stream_per_vertex(&Particle::size)->to_location(1),
			avk::descriptor_binding(0,0, mHostParticleBuffer->as_uniform_buffer()),
			// Just use the main window's renderpass for this pipeline:
			avk::context().main_window()->get_renderpass()
		);
		
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
		// TODO stop & join runtime and issue-particle-tick threads 
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
		
		std::array<Particle, 3> pts = {
			Particle{glm::vec3(-0.5, -0.5, 0.0), glm::vec3(0.0, 0.0, 0.0), -1.0, 0.5},
			Particle{glm::vec3(0.5, 0.5, 0.0), glm::vec3(0.0, 0.0, 0.0), 0.5, 0.5},
			Particle{glm::vec3(0.5, -0.5, 0.0), glm::vec3(0.0, 0.0, 0.0), 0.5, 0.5}
		};

		auto pbufferSem = avk::context().record_and_submit_with_semaphore(
			{ mHostParticleBuffer->fill(pts.data(),0) }, *mRenderQueue, avk::stage::all_commands);

		avk::context().record({
			// Begin and end one renderpass:
			avk::command::render_pass(mRenderPipeline->renderpass_reference(), avk::context().main_window()->current_backbuffer_reference(), {
					avk::command::bind_pipeline(mRenderPipeline.as_reference()),
					avk::command::bind_descriptors(mRenderPipeline->layout(), mDescriptorCache->get_or_create_descriptor_sets({
						avk::descriptor_binding(0, 0, mHostParticleBuffer)
					})),
					avk::command::draw(pts.size() * 6, pts.size() * 2, 0u, 0u)
				})
			})
			.into_command_buffer(cmdBfr)
			.then_submit_to(*mRenderQueue)
			.waiting_for(imageAvailableSemaphore >> avk::stage::color_attachment_output)
			.waiting_for(pbufferSem >> avk::stage::vertex_shader)
			.submit();
		cmdBfr->handle_lifetime_of(pbufferSem);

		// Use a convenience function of avk::window to take care of the command buffer's lifetime:
		// It will get deleted in the future after #concurrent-frames have passed by.
		avk::context().main_window()->handle_lifetime(std::move(cmdBfr));
	}

private: // v== Member variables ==v

	avk::queue* mRenderQueue;
	avk::queue* mParticleQueue;
	avk::buffer mParticleComputeBuffer;
	avk::buffer mParticleIntermediateBuffer;
	avk::buffer mHostParticleBuffer;
	std::vector<avk::buffer> mParticleVertexBuffers;
	avk::buffer mMetadataBuffer;
	Metadata<10> mMetadata;
	avk::graphics_pipeline mRenderPipeline;
	avk::compute_pipeline mParticlePipeline;

	avk::descriptor_cache mDescriptorCache;

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
