#include <cg_base.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace cgb
{
	void imgui_manager::initialize()
	{
		LOG_DEBUG_VERBOSE("Setting up IMGUI...");

		// Get the main window's handle:
		auto* wnd = cgb::context().main_window();

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// Setup Platform/Renderer bindings
		ImGui_ImplGlfw_InitForVulkan(wnd->handle()->mHandle, true); // TODO: Don't install callbacks (get rid of them during 'fixed/varying-input Umstellung DOUBLECHECK')

		ImGui_ImplVulkan_InitInfo init_info = {};
	    init_info.Instance = context().vulkan_instance();
	    init_info.PhysicalDevice = context().physical_device();
	    init_info.Device = context().logical_device();
	    init_info.QueueFamily = context().graphics_queue().family_index();
	    init_info.Queue = context().graphics_queue().handle();
	    init_info.PipelineCache = nullptr; // TODO: Maybe use a pipeline cache?

		const uint32_t magicImguiFactor = 1000;
		auto allocRequest = descriptor_alloc_request::create({});
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{vk::DescriptorType::eSampler,				 magicImguiFactor});
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, magicImguiFactor});
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage,		 magicImguiFactor});
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage,		 magicImguiFactor});
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{vk::DescriptorType::eUniformTexelBuffer,	 magicImguiFactor});
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{vk::DescriptorType::eStorageTexelBuffer,	 magicImguiFactor});
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer,		 magicImguiFactor});
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer,		 magicImguiFactor});
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{vk::DescriptorType::eUniformBufferDynamic, magicImguiFactor}); // TODO: Q1: Is this really required? Q2: Why is the type not abstracted through cgb::binding?
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{vk::DescriptorType::eStorageBufferDynamic, magicImguiFactor}); // TODO: Q1: Is this really required? Q2: Why is the type not abstracted through cgb::binding?
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{vk::DescriptorType::eInputAttachment,		 magicImguiFactor}); // TODO: Q1: Is this really required? Q2: Why is the type not abstracted through cgb::binding?
		allocRequest.set_num_sets(allocRequest.accumulated_pool_sizes().size() * magicImguiFactor);
		mDescriptorPool = cgb::context().get_descriptor_pool_for_layouts(allocRequest, 'imgu');
		
	    init_info.DescriptorPool = mDescriptorPool->handle();
	    init_info.Allocator = nullptr; // TODO: Maybe use an allocator?

		// MinImageCount and ImageCount are related to the swapchain images. These are not Dear ImGui specific properties and your
		// engine should expose them. ImageCount lets Dear ImGui know how many framebuffers and resources in general it should
		// allocate. MinImageCount is not actually used even though there is a check at init time that its value is greater than 1.
		// Source: https://frguthmann.github.io/posts/vulkan_imgui/
	    init_info.MinImageCount = std::max(2u, static_cast<uint32_t>(wnd->number_of_in_flight_frames()));
	    init_info.ImageCount = std::max(init_info.MinImageCount, static_cast<uint32_t>(wnd->number_of_in_flight_frames()));
	    init_info.CheckVkResultFn = cgb::context().check_vk_result;

		mRenderpass = renderpass_t::create({
			cgb::attachment::create_color(image_format::from_window_color_buffer(wnd)).set_load_operation(cfg::attachment_load_operation::load)
		});

		// Init it:
	    ImGui_ImplVulkan_Init(&init_info, mRenderpass->handle());

		// Upload fonts:
		auto cmdBfr = cgb::context().graphics_queue().create_single_use_command_buffer(); // TODO: Graphics queue?
		cmdBfr->begin_recording();
		ImGui_ImplVulkan_CreateFontsTexture(cmdBfr->handle());
		cmdBfr->end_recording();
		cmdBfr->set_custom_deleter([]() { ImGui_ImplVulkan_DestroyFontUploadObjects(); });
		auto semaph = cgb::context().graphics_queue().submit_and_handle_with_semaphore(std::move(cmdBfr));
		wnd->set_extra_semaphore_dependency(std::move(semaph));
	}

	
	void imgui_manager::update()
	{
		ImGuiIO& io = ImGui::GetIO();
		IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");
		auto wndSize = cgb::context().main_window()->resolution(); // TODO: What about multiple windows?
		io.DisplaySize = ImVec2((float)wndSize.x, (float)wndSize.y);
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f); // TODO: If the framebuffer has a different resolution as the window
	    io.DeltaTime = cgb::time().delta_time();
		io.MouseDown[0] = input().mouse_button_down(0);
		io.MouseDown[1] = input().mouse_button_down(1);
		io.MouseDown[2] = input().mouse_button_down(2);
		io.MouseDown[3] = input().mouse_button_down(3);
		io.MouseDown[4] = input().mouse_button_down(4);
		const auto cursorPos = input().cursor_position();
		io.MousePos = ImVec2(cursorPos.x, cursorPos.y);
	}

	void imgui_manager::render()
	{
		//ImGui_ImplVulkan_NewFrame();
		//ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::ShowDemoWindow();
		ImGui::Render();
		auto cmdBfr = cgb::context().graphics_queue().create_single_use_command_buffer();
		cmdBfr->begin_recording();
		cmdBfr->begin_render_pass_for_framebuffer(mRenderpass, cgb::context().main_window());
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBfr->handle());
		cmdBfr->end_render_pass();
		cmdBfr->end_recording();
		submit_command_buffer_ownership(std::move(cmdBfr));
	}

	void imgui_manager::finalize()
	{
		ImGui_ImplVulkan_Shutdown();
	    ImGui_ImplGlfw_Shutdown();
	    ImGui::DestroyContext();
	}

}
