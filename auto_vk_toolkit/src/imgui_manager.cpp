#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#ifdef _WIN32
//#include "imgui_impl_win32.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>   // for glfwGetWin32Window

#include "imgui_manager.hpp"
#include "composition_interface.hpp"
#include "vk_convenience_functions.hpp"
#include "timer_interface.hpp"

namespace avk
{
	void imgui_manager::initialize()
	{
		LOG_DEBUG_VERBOSE("Setting up IMGUI...");

		// Get the main window's handle:
		auto* wnd = context().main_window();

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsClassic();

		// Try to determine the display scale s.t. we can scale the font and the UI accordingly:
		auto assignedMonitor = wnd->monitor();
	    std::atomic_bool contentScaleRetrieved = false;
		float contentScale = 0.f;
		context().dispatch_to_main_thread([&contentScaleRetrieved, assignedMonitor, &contentScale]{
			auto* monitorHandle = assignedMonitor.has_value() ? assignedMonitor->mHandle : glfwGetPrimaryMonitor();
			float xscale, yscale;
			glfwGetMonitorContentScale(monitorHandle, &xscale, &yscale);
			contentScale = (xscale + yscale) * 0.5; // Note: xscale and yscale are probably the same
		    contentScaleRetrieved = true;
		});
		context().signal_waiting_main_thread();
		while(!contentScaleRetrieved) { LOG_DEBUG("Waiting for main thread..."); }

		const float baseFontSize = 15.f;
		float fontSize = glm::round(baseFontSize * contentScale);
		// Scale the UI according to the rounded font size:
		float uiScale  = fontSize / baseFontSize;

		if (mCustomTtfFont.empty()) {
			io.Fonts->AddFontDefault();
		}
		else {
			auto  font_cfg = ImFontConfig();
			font_cfg.FontDataOwnedByAtlas = false;
			ImFormatString(font_cfg.Name, IM_ARRAYSIZE(font_cfg.Name), "%s, %.0fpx", mCustomTtfFont.c_str(), fontSize);
			size_t data_size;
			io.Fonts->AddFontFromFileTTF(mCustomTtfFont.c_str(), fontSize, &font_cfg, nullptr);
		}

		auto& style                              = ImGui::GetStyle();
        style.Colors[ImGuiCol_TitleBg]           = ImVec4(0.0f, 150.0f / 255.f, 169.0f / 255.f, 159.f / 255.f);
        style.Colors[ImGuiCol_TitleBgActive]     = ImVec4(0.0f, 166.0f / 255.f, 187.0f / 255.f, 244.f / 255.f);
        style.Colors[ImGuiCol_MenuBarBg]         = ImVec4(0.0f, 150.0f / 255.f, 169.0f / 255.f, 159.f / 255.f);
        style.Colors[ImGuiCol_TitleBgCollapsed]  = ImVec4(0.0f, 111.0f / 255.f, 125.0f / 255.f, 110.f / 255.f);
        style.Colors[ImGuiCol_Header]            = ImVec4(0.0f, 150.0f / 255.f, 169.0f / 255.f, 107.f / 255.f);
        style.Colors[ImGuiCol_CheckMark]         = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_Button]            = ImVec4(0.0f, 150.0f / 255.f, 169.0f / 255.f, 159.f / 255.f);
        style.Colors[ImGuiCol_ButtonHovered]     = ImVec4(0.0f, 150.0f / 255.f, 169.0f / 255.f, 159.f / 255.f);
        style.Colors[ImGuiCol_ButtonActive]      = ImVec4(216.f / 255.f, 42.f / 255.f, 99.f / 255.f, 242.f / 255.f);
        style.ChildRounding                      = 2.f;
        style.FrameRounding                      = 2.f;
        style.GrabRounding                       = 2.f;
        style.PopupRounding                      = 2.f;
        style.PopupRounding                      = 2.f;
        style.ScrollbarRounding                  = 2.f;
        style.TabRounding                        = 2.f;
        style.WindowRounding                     = 2.f;
		style.ScaleAllSizes(uiScale);

		if (mAlterSettingsBeforeInitialization) {
			mAlterSettingsBeforeInitialization(uiScale); // allow the user to change the style
		}

		// Setup Platform/Renderer bindings
		ImGui_ImplGlfw_InitForVulkan(wnd->handle()->mHandle, true); // TODO: Don't install callbacks (get rid of them during 'fixed/varying-input Umstellung DOUBLECHECK')

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = context().vulkan_instance();
		init_info.PhysicalDevice = context().physical_device();
		init_info.Device = context().device();
		assert(mQueue);
		init_info.QueueFamily = mQueue->family_index();
		init_info.Queue = mQueue->handle();
		init_info.PipelineCache = nullptr; // TODO: Maybe use a pipeline cache?

		// This factor is set to 1000 in the imgui example code but after looking through the vulkan backend code, we never
		// allocate more than one descriptor set, therefore setting this to 1 should be sufficient.
		const uint32_t magicImguiFactor = 1;
		auto allocRequest = avk::descriptor_alloc_request{};
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{ vk::DescriptorType::eSampler,              magicImguiFactor });
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, std::max(magicImguiFactor, 32u) }); // User could alloc several of these via imgui_manager::get_or_create_texture_descriptor
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{ vk::DescriptorType::eSampledImage,         magicImguiFactor });
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{ vk::DescriptorType::eStorageImage,         magicImguiFactor });
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{ vk::DescriptorType::eUniformTexelBuffer,   magicImguiFactor });
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{ vk::DescriptorType::eStorageTexelBuffer,   magicImguiFactor });
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer,        magicImguiFactor });
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{ vk::DescriptorType::eStorageBuffer,        magicImguiFactor });
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBufferDynamic, magicImguiFactor }); // TODO: Q1: Is this really required? Q2: Why is the type not abstracted through avk::binding?
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{ vk::DescriptorType::eStorageBufferDynamic, magicImguiFactor }); // TODO: Q1: Is this really required? Q2: Why is the type not abstracted through avk::binding?
		allocRequest.add_size_requirements(vk::DescriptorPoolSize{ vk::DescriptorType::eInputAttachment,      magicImguiFactor });
		allocRequest.set_num_sets(static_cast<uint32_t>(allocRequest.accumulated_pool_sizes().size() * magicImguiFactor));
		mDescriptorPool = context().create_descriptor_pool(allocRequest.accumulated_pool_sizes(), allocRequest.num_sets());

		// DescriptorSet chache for user textures
		mImTextureDescriptorCache = context().create_descriptor_cache("imgui_manager's texture descriptor cache");

		init_info.DescriptorPool = mDescriptorPool.handle();
		init_info.Allocator = nullptr; // TODO: Maybe use an allocator?

		mCommandPool = &context().get_command_pool_for_resettable_command_buffers(*mQueue).get(); // TODO: Support other queues!
		mCommandBuffers = mCommandPool->alloc_command_buffers(static_cast<uint32_t>(wnd->number_of_frames_in_flight()));
		for (avk::window::frame_id_t i = 0; i < wnd->number_of_frames_in_flight(); ++i) {
			mRenderFinishedSemaphores.push_back(context().create_semaphore());
			mRenderFinishedSemaphores.back().enable_shared_ownership();
		}

		// MinImageCount and ImageCount are related to the swapchain images. These are not Dear ImGui specific properties and your
		// engine should expose them. ImageCount lets Dear ImGui know how many framebuffers and resources in general it should
		// allocate. MinImageCount is not actually used even though there is a check at init time that its value is greater than 1.
		// Source: https://frguthmann.github.io/posts/vulkan_imgui/
		// ImGui has a hard-coded floor for MinImageCount which is 2.
		// Take the max of min image count supported by the phys. device and imgui:
		auto surfaceCap = context().physical_device().getSurfaceCapabilitiesKHR(wnd->surface());
		init_info.MinImageCount = std::max(2u, surfaceCap.minImageCount);
		init_info.ImageCount = std::max(init_info.MinImageCount, std::max(static_cast<uint32_t>(wnd->get_config_number_of_concurrent_frames()), wnd->get_config_number_of_presentable_images()));
		init_info.CheckVkResultFn = context().check_vk_result;

		// copy current state of init_info in for later use
		// this shenanigans is necessary for ImGui to keep functioning when certain rendering properties (renderpass) are changed (and to give it new image count)
		auto restartImGui = [this, wnd, init_info]() {
			ImGui_ImplVulkan_Shutdown(); // shut imgui down and restart with base init_info
			ImGui_ImplVulkan_InitInfo new_init_info = init_info; // can't be temp
			mDescriptorPool.reset();
			new_init_info.ImageCount = std::max(init_info.MinImageCount, std::max(static_cast<uint32_t>(wnd->get_config_number_of_concurrent_frames()), wnd->get_config_number_of_presentable_images())); // opportunity to update image count
			ImGui_ImplVulkan_Init(&new_init_info, this->mRenderpass->handle()); // restart imgui
			// Have to upload fonts just like the first time:
			upload_fonts();

			// Re-create all the command buffers because the number of concurrent frames could have changed:
			mCommandBuffers = mCommandPool->alloc_command_buffers(static_cast<uint32_t>(wnd->number_of_frames_in_flight()));

			// Re-create all the semaphores because the number of concurrent frames could have changed:
			mRenderFinishedSemaphores.clear();
			for (avk::window::frame_id_t i = 0; i < wnd->number_of_frames_in_flight(); ++i) {
				mRenderFinishedSemaphores.push_back(context().create_semaphore());
				mRenderFinishedSemaphores.back().enable_shared_ownership();
			}
		};

		// set up an updater
		mUpdater.emplace();
		if (!mRenderpass.has_value()) { // Not specified in the constructor => create a default one
			construct_render_pass();

			mUpdater->on(avk::swapchain_format_changed_event(wnd),
				avk::swapchain_additional_attachments_changed_event(wnd)
			).invoke([this, restartImGui]() {
				ImGui::EndFrame(); //end previous (not rendered) frame
				construct_render_pass(); // reconstruct render pass
				restartImGui();
				ImGui::NewFrame(); // got to start a new frame since ImGui::Render is next
			});
		}
		mUpdater->on(avk::concurrent_frames_count_changed_event(wnd)).invoke([restartImGui]() {
			ImGui::EndFrame(); //end previous (not rendered) frame
			restartImGui();
			ImGui::NewFrame(); // got to start a new frame since ImGui::Render is next
		});


#if defined(_WIN32)
        context().dispatch_to_main_thread([]() {
            auto wndHandle = context().main_window()->handle()->mHandle;
            auto hwnd = (void*)glfwGetWin32Window(wndHandle);
            ImGui::GetMainViewport()->PlatformHandleRaw = hwnd;
		});
#endif

		// Init it:
		ImGui_ImplVulkan_Init(&init_info, mRenderpass->handle());

		// Setup back-end capabilities flags
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
		//io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
		io.BackendPlatformName = "imgui_impl_glfw";

		//io.SetClipboardTextFn = ImGui_ImplGlfw_SetClipboardText; // TODO clipboard abstraction via avk::input()
		//io.GetClipboardTextFn = ImGui_ImplGlfw_GetClipboardText;
		//io.ClipboardUserData = g_Window;
		// Upload fonts:
		upload_fonts();
	}

	void imgui_manager::update()
	{
		ImGuiIO& io = ImGui::GetIO();
		IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");
		auto wndSize = context().main_window()->resolution(); // TODO: What about multiple windows?
		io.DisplaySize = ImVec2((float)wndSize.x, (float)wndSize.y);
		io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f); // TODO: If the framebuffer has a different resolution as the window
		io.DeltaTime = avk::time().delta_time();

		mOccupyMouseLastFrame = mOccupyMouse;
		if (mUserInteractionEnabled) {
			mOccupyMouse = io.WantCaptureMouse || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);

			// Cursor position:
			static const auto input = []() -> input_buffer& { return composition_interface::current()->input(); };

			const auto cursorPos = input().cursor_position();
			io.AddMousePosEvent(static_cast<float>(cursorPos.x), static_cast<float>(cursorPos.y));

			// Mouse buttons:
			for (int button = 0; button < 5; ++button) {
				io.AddMouseButtonEvent(button, input().mouse_button_down(button));
			}

			// Scroll position:
			io.AddMouseWheelEvent(static_cast<float>(input().scroll_delta().x),
				static_cast<float>(input().scroll_delta().y));

			// Mouse cursor:
			if (!input().is_cursor_disabled()) {
				const auto mouseCursorCurValue = ImGui::GetMouseCursor();
				if (static_cast<int>(mouseCursorCurValue) != mMouseCursorPreviousValue) {
					switch (mouseCursorCurValue) {
					case ImGuiMouseCursor_None:
						input().set_cursor_mode(cursor::cursor_hidden);
						break;
					case ImGuiMouseCursor_Arrow:
						input().set_cursor_mode(cursor::arrow_cursor);
						break;
					case ImGuiMouseCursor_TextInput:
						input().set_cursor_mode(cursor::ibeam_cursor);
						break;
					case ImGuiMouseCursor_ResizeAll:
						input().set_cursor_mode(cursor::arrow_cursor);
						break;
					case ImGuiMouseCursor_ResizeNS:
						input().set_cursor_mode(cursor::vertical_resize_cursor);
						break;
					case ImGuiMouseCursor_ResizeEW:
						input().set_cursor_mode(cursor::horizontal_resize_cursor);
						break;
					case ImGuiMouseCursor_ResizeNESW:
						input().set_cursor_mode(cursor::arrow_cursor);
						break;
					case ImGuiMouseCursor_ResizeNWSE:
						input().set_cursor_mode(cursor::arrow_cursor);
						break;
					case ImGuiMouseCursor_Hand:
						input().set_cursor_mode(cursor::hand_cursor);
						break;
					case ImGuiMouseCursor_NotAllowed:
						input().set_cursor_mode(cursor::arrow_cursor);
						break;
					default:
						input().set_cursor_mode(cursor::arrow_cursor);
						break;
					}
					mMouseCursorPreviousValue = static_cast<int>(mouseCursorCurValue);
				}
			}

			// Update keys:
			io.AddKeyEvent(ImGuiKey_Tab, input().key_down(key_code::tab));
			io.AddKeyEvent(ImGuiKey_LeftArrow, input().key_down(key_code::left));
			io.AddKeyEvent(ImGuiKey_RightArrow, input().key_down(key_code::right));
			io.AddKeyEvent(ImGuiKey_UpArrow, input().key_down(key_code::up));
			io.AddKeyEvent(ImGuiKey_DownArrow, input().key_down(key_code::down));
			io.AddKeyEvent(ImGuiKey_PageUp, input().key_down(key_code::page_up));
			io.AddKeyEvent(ImGuiKey_PageDown, input().key_down(key_code::page_down));
			io.AddKeyEvent(ImGuiKey_Home, input().key_down(key_code::home));
			io.AddKeyEvent(ImGuiKey_End, input().key_down(key_code::end));
			io.AddKeyEvent(ImGuiKey_Insert, input().key_down(key_code::insert));
			io.AddKeyEvent(ImGuiKey_Delete, input().key_down(key_code::del));
			io.AddKeyEvent(ImGuiKey_Backspace, input().key_down(key_code::backspace));
			io.AddKeyEvent(ImGuiKey_Space, input().key_down(key_code::space));
			io.AddKeyEvent(ImGuiKey_Enter, input().key_down(key_code::enter));
			io.AddKeyEvent(ImGuiKey_Escape, input().key_down(key_code::escape));
			io.AddKeyEvent(ImGuiKey_KeypadEnter, input().key_down(key_code::numpad_enter));
			io.AddKeyEvent(ImGuiKey_A, input().key_down(key_code::a));
			io.AddKeyEvent(ImGuiKey_C, input().key_down(key_code::c));
			io.AddKeyEvent(ImGuiKey_V, input().key_down(key_code::v));
			io.AddKeyEvent(ImGuiKey_X, input().key_down(key_code::x));
			io.AddKeyEvent(ImGuiKey_Y, input().key_down(key_code::y));
			io.AddKeyEvent(ImGuiKey_Z, input().key_down(key_code::z));

			// Modifiers are not reliable across systems
			io.KeyCtrl = input().key_down(key_code::left_control) || input().key_down(key_code::right_control);
			io.KeyShift = input().key_down(key_code::left_shift) || input().key_down(key_code::right_shift);
			io.KeyAlt = input().key_down(key_code::left_alt) || input().key_down(key_code::right_alt);
			io.KeySuper = input().key_down(key_code::left_super) || input().key_down(key_code::right_super);
			io.AddKeyEvent(ImGuiMod_Ctrl, io.KeyCtrl);
			io.AddKeyEvent(ImGuiMod_Shift, io.KeyShift);
			io.AddKeyEvent(ImGuiMod_Alt, io.KeyAlt);
			io.AddKeyEvent(ImGuiMod_Super, io.KeySuper);

			// Characters:
			for (auto c : input().entered_characters()) {
				io.AddInputCharacter(c);
			}
		}
		else {
			mOccupyMouse = false;
		}
		
		// start of new frame and callback invocations have to be in the update() call of the invokee,
		// ... to give the updater an opportunity to clean up (callbacks themselves may cause update events)
		mAlreadyRendered = false;
		ImGui_ImplVulkan_NewFrame();
		ImGui::NewFrame();
	}

	void imgui_manager::render_into_command_buffer(avk::command_buffer_t& aCommandBuffer, std::optional<std::reference_wrapper<const avk::framebuffer_t>> aTargetFramebuffer)
	{
		for (auto& cb : mCallback) {
			cb();
		}

		auto& cmdBfr = aCommandBuffer;

		auto mainWnd = context().main_window(); // TODO: ImGui shall not only support main_mindow, but all windows!

		// if no invokee has written on the attachment (no previous render calls this frame),
		// reset layout (cannot be "store_in_presentable_format").
		if (!mainWnd->has_consumed_current_image_available_semaphore()) {
			cmdBfr.record(avk::command::render_pass(*mClearRenderpass, aTargetFramebuffer.value_or(std::ref(mainWnd->current_backbuffer_reference())).get())); // Begin and end without nested commands
		}

		assert(mRenderpass.has_value());
		cmdBfr.record(avk::command::begin_render_pass_for_framebuffer(*mRenderpass, aTargetFramebuffer.value_or(std::cref(mainWnd->current_backbuffer_reference())).get()));

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBfr.handle());

		cmdBfr.record(avk::command::end_render_pass());

		mAlreadyRendered = true;
	}

	avk::command::action_type_command imgui_manager::render_command(std::optional<avk::resource_argument<avk::framebuffer_t>> aTargetFramebuffer)
	{
		auto actionTypeCommand = avk::command::action_type_command{
			// According to imgui_impl_vulkan.cpp, the code of the relevant fragment shader is:
			//
			// #version 450 core
			// layout(location = 0) out vec4 fColor;
			// layout(set = 0, binding = 0) uniform sampler2D sTexture;
			// layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
			// void main()
			// {
			// 	fColor = In.Color * texture(sTexture, In.UV.st);
			// }
			//
			// Therefore, sync with sampled reads in fragment shaders.
			// 
			// ImGui seems to not use any depth testing/writing.
			// 
			// ImGui's color attachment write must still wait for preceding color attachment writes because
			// the user interface shall be drawn on top of the rest.
			//
			// I'm just not sure about the vertex attribute input. But I think the buffers are in host-visible
			// memory region, get updated on the host, and transfered to the GPU every frame.
			// IF ^ this is true, then the sync hint should be fine. Otherwise it fails to synchronize with previous data transfer.
			// 
			avk::sync::sync_hint {
				{{ // What previous commands must synchronize with:
					vk::PipelineStageFlagBits2KHR::eFragmentShader | vk::PipelineStageFlagBits2KHR::eColorAttachmentOutput,
					vk::AccessFlagBits2KHR::eShaderSampledRead | vk::AccessFlagBits2KHR::eColorAttachmentWrite
				}},
				{{ // What subsequent commands must synchronize with:
					vk::PipelineStageFlagBits2KHR::eColorAttachmentOutput,
					vk::AccessFlagBits2KHR::eColorAttachmentWrite
				}}
			},
			{}, // no resource-specific sync hints
			[
				this,
				lMainWnd = context().main_window(), // TODO: ImGui shall not only support main_mindow, but all windows!
				lFramebufferPtr = aTargetFramebuffer.has_value()
					? &aTargetFramebuffer.value().get_const_reference()
					: &context().main_window()->current_backbuffer_reference()
			](avk::command_buffer_t& cmdBfr) {
				for (auto& cb : mCallback) {
					cb();
				}
				
				// if no invokee has written on the attachment (no previous render calls this frame),
				// reset layout (cannot be "store_in_presentable_format").
				if (!lMainWnd->has_consumed_current_image_available_semaphore()) {
					cmdBfr.record(avk::command::render_pass(mClearRenderpass.as_reference(), *lFramebufferPtr)); // Begin and end without nested commands
				}

				assert(mRenderpass.has_value());
				cmdBfr.record(avk::command::begin_render_pass_for_framebuffer(mRenderpass.as_reference(), *lFramebufferPtr));

				ImGui::Render();
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBfr.handle());

				cmdBfr.record(avk::command::end_render_pass());

				mAlreadyRendered = true;
			}
		};

		if (aTargetFramebuffer.has_value() && aTargetFramebuffer->is_ownership()) {
			actionTypeCommand.mEndFun = [
				lFramebuffer = aTargetFramebuffer->move_ownership_or_get_empty()
			](avk::command_buffer_t& cb) mutable {
				let_it_handle_lifetime_of(cb, lFramebuffer);
			};
		}

		return actionTypeCommand;
	}

	void imgui_manager::render()
	{
		if (mAlreadyRendered) {
			return;
		}

		auto mainWnd = context().main_window(); // TODO: ImGui shall not only support main_mindow, but all windows!
		const auto ifi = mainWnd->current_in_flight_index();
		auto& cmdBfr = mCommandBuffers[ifi];

		cmdBfr->reset();

		cmdBfr->begin_recording();
		render_into_command_buffer(cmdBfr.as_reference());
		cmdBfr->end_recording();

		auto submission = mQueue->submit(cmdBfr.as_reference())
			.signaling_upon_completion(avk::stage::color_attachment_output >> mRenderFinishedSemaphores[ifi])
			.store_for_now();

		// If this is the first render call (other invokees are disabled or only ImGui renders),
		// then consume imageAvailableSemaphore.
		if (!mainWnd->has_consumed_current_image_available_semaphore()) {
			submission
				.waiting_for(mainWnd->consume_current_image_available_semaphore() >> avk::stage::color_attachment_output);
		}

		// This is usually the last call, so someone needs to use the fence:
		// TODO: How to check if we have another invokee after this one?!
		if (!mainWnd->has_used_current_frame_finished_fence()) {
			submission
				.signaling_upon_completion(mainWnd->use_current_frame_finished_fence());
		}

		submission.submit();

		//                        As far as ImGui is concerned, the next frame using the same target image must wait before color attachment output:
		mainWnd->add_present_dependency_for_current_frame(mRenderFinishedSemaphores[ifi]);

		// Just let submission go out of scope => will submit in destructor, that's fine.
	}

	void imgui_manager::finalize()
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void imgui_manager::enable_user_interaction(bool aEnableOrNot)
	{
		mUserInteractionEnabled = aEnableOrNot;
	}

	void imgui_manager::upload_fonts()
	{
		auto cmdBfr = context().get_command_pool_for_single_use_command_buffers(*mQueue)->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		cmdBfr->begin_recording();
		ImGui_ImplVulkan_CreateFontsTexture(cmdBfr->handle());
		cmdBfr->end_recording();
		cmdBfr->set_custom_deleter([]() { ImGui_ImplVulkan_DestroyFontUploadObjects(); });

		if (mUsingSemaphoreInsteadOfFenceForFontUpload) {
			auto semaphore = context().create_semaphore();
			mQueue->submit(cmdBfr.as_reference())
				.signaling_upon_completion(avk::stage::transfer >> semaphore);

			// Let the semaphore handle the command buffer's lifetime:
			semaphore->handle_lifetime_of(std::move(cmdBfr));

			// The following is not totally correct, i.e., living on the edge:
			auto* mainWnd = context().main_window();
			mainWnd->add_present_dependency_for_current_frame(std::move(semaphore));
		}
		else {
			auto fen = context().create_fence();
			mQueue->submit(cmdBfr.as_reference())
				.signaling_upon_completion(fen);

			// This is totally correct, but incurs a fence wait:
			fen->wait_until_signalled();
		}
	}

	void imgui_manager::construct_render_pass()
	{
		using namespace avk;

		auto* wnd = context().main_window();
		std::vector<attachment> attachments;
		attachments.push_back(attachment::declare(format_from_window_color_buffer(wnd), on_load::load.from_previous_layout(mIncomingLayout), usage::color(0), on_store::store.in_layout(layout::present_src)));
		for (auto a : wnd->get_additional_back_buffer_attachments()) {
			// Well... who would have guessed the following (and, who understands??):
			//
			// Load and store operations often cause synchronization errors
			//  - LOAD_OP_DONT_CARE generates WRITE accesses to your attachments
			//
			// I mean... yeah! Sure... :|
			// Source: https://www.lunarg.com/wp-content/uploads/2021/08/Vulkan-Synchronization-SIGGRAPH-2021.pdf
			//
			// Therefore ...
			// 
			a.mLoadOperation = on_load::dont_care;
			a.mStoreOperation = on_store::dont_care;
			attachments.push_back(a);
		}
		auto newRenderpass = context().create_renderpass(
			attachments,
			{
				subpass_dependency(
					subpass::external >> subpass::index(0),
					// ... we have to synchronize all these stages with color+dept_stencil write access:
					stage::color_attachment_output   >>   stage::color_attachment_output,
					access::none                     >>   access::color_attachment_read | access::color_attachment_write // read && write due to layout transition
				),
				subpass_dependency(
					subpass::index(0) >> subpass::external,
					stage::color_attachment_output   >>   stage::color_attachment_output,  // assume semaphore afterwards
					access::color_attachment_write   >>   access::none                     //  ^ ...but still, gotta synchronize image layout transition with subsequent presentKHR :ohgodwhy:
				)
			}
		);

		// setup render pass for the case where the invokee does not write anything on the backbuffer (and clean it)
		attachments[0] = avk::attachment::declare(format_from_window_color_buffer(wnd), on_load::clear.from_previous_layout(avk::layout::undefined), usage::color(0), on_store::store);
		auto newClearRenderpass = context().create_renderpass(
			attachments
			, {
				subpass_dependency(
					subpass::external >> subpass::index(0),
					stage::color_attachment_output   >>   stage::color_attachment_output,
					access::none                     >>   access::color_attachment_read | access::color_attachment_write // read && write due to layout transition
				),
				subpass_dependency(
					subpass::index(0) >> subpass::external,
					stage::color_attachment_output >> stage::color_attachment_output,  // assume semaphore afterwards
					access::color_attachment_write >> access::none                     //  ^ ...but still, gotta synchronize image layout transition with subsequent presentKHR :ohgodwhy:
				)
			}
		);

		auto lifetimeHandlerLambda = [wnd](avk::renderpass&& rp) { wnd->handle_lifetime(std::move(rp)); };
		avk::assign_and_lifetime_handle_previous(mRenderpass, std::move(newRenderpass), lifetimeHandlerLambda);
		avk::assign_and_lifetime_handle_previous(mClearRenderpass, std::move(newClearRenderpass), lifetimeHandlerLambda);
	}

	ImTextureID imgui_manager::get_or_create_texture_descriptor(const avk::image_sampler_t& aImageSampler, avk::layout::image_layout aImageLayout)
	{
		std::vector<avk::descriptor_set> sets = mImTextureDescriptorCache->get_or_create_descriptor_sets({
			avk::descriptor_binding(0, 0, aImageSampler.as_combined_image_sampler(aImageLayout), avk::shader_type::fragment)
		});

		// The vector should never contain more than 1 DescriptorSet for the provided image_sampler
		assert(sets.size() == 1);

		// Return the first DescriptorSet as ImTextureID
		return (ImTextureID)sets[0].handle();
	}
}
