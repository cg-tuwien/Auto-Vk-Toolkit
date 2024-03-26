#pragma once

#include "invokee.hpp"

// Forward-declare ImGui's ImTextureID type:
typedef void* ImTextureID;

namespace avk
{
	class imgui_manager : public invokee
	{
	public:
		/**	Create an ImGui manager element.
		 *	@param		aName								You *can* give it a name, but you can also leave it at the default name "imgui_manager".
		 *	@param		aAlterSettingsBeforeInitialization	Allows to modify the ImGui style, e.g., like follows:
		 *													[](float uiScale) {
		 *														auto& style = ImGui::GetStyle();
		 *														style = ImGuiStyle(); // reset to default style (for non-color settings, like rounded corners)
		 *														ImGui::StyleColorsClassic(); // change color theme
		 *														style.ScaleAllSizes(uiScale); // and scale
		 *													}
		 */
		imgui_manager(avk::queue& aQueueToSubmitTo, std::string aName = "imgui_manager", std::optional<avk::renderpass> aRenderpassToUse = {}, std::function<void(float)> aAlterSettingsBeforeInitialization = {})
			: invokee(std::move(aName))
			, mQueue { &aQueueToSubmitTo }
			, mUserInteractionEnabled{ true }
			, mAlterSettingsBeforeInitialization{ std::move(aAlterSettingsBeforeInitialization) }
			, mIncomingLayout{ layout::color_attachment_optimal }
		{
			if (aRenderpassToUse.has_value()) {
				mRenderpass = std::move(aRenderpassToUse.value());
			}
		}

		/** ImGui should run very late => hence 100000 */
		int execution_order() const override { return 100000; }

		void initialize() override;

		void update() override;

		/**	This method can be used to prematurely render ImGui into the given command buffer.
		 *	If this method is invoked before ::render, then ::render will NOT create a new command buffer
		 *	and submit that to the queue.
		 *
		 *	A prime example for following this approach might be to streamline rendering into few command buffers,
		 *	but it can also be helpful for validation synchronization, which is only able to perform its checks
		 *	WITHIN command buffers---not across different command buffers.
		 *
		 *	The flag mAlreadyRendered is set in ::update and evaluated in ::render to determine if ::render
		 *	shall create a new command buffer and render into it and submit it to the queue.
		 */
		void render_into_command_buffer(avk::command_buffer_t& aCommandBuffer, std::optional<std::reference_wrapper<const avk::framebuffer_t>> aTargetFramebuffer = {});

		/**	This method can be used to prematurely render ImGui into the given command buffer---same as via
		 *  render_into_command_buffer, but now handled through an avk::command.
		 *
		 *	If the command is executed before ::render, then ::render will NOT create a new command buffer
		 *	and submit that to the queue.
		 *
		 *	A prime example for following this approach might be to streamline rendering into few command buffers,
		 *	but it can also be helpful for validation synchronization, which is only able to perform its checks
		 *	WITHIN command buffers---not across different command buffers.
		 *
		 *	The flag mAlreadyRendered is set in ::update and evaluated in ::render to determine if ::render
		 *	shall create a new command buffer and render into it and submit it to the queue.
		 */
		avk::command::action_type_command render_command(std::optional<avk::resource_argument<avk::framebuffer_t>> aTargetFramebuffer = {});

		void render() override;

		void finalize() override;

		template <typename F>
		void add_callback(F&& aCallback)
		{
			mCallback.emplace_back(std::forward<F>(aCallback));
		}

		void enable_user_interaction(bool aEnableOrNot);
		bool is_user_interaction_enabled() const { return mUserInteractionEnabled; }

		/**	Get or create a texture Descriptor for use in ImGui
		 *	
		 *  This function creates and caches or returns an already cached DescriptorSet for the provided `avk::image_sampler`
		 *
		 *	@param	aImageSampler		An image sampler, that shall be rendered with ImGui
		 *	@param	aImageLayout		The image layout that the image is expected to be given in
		 *  @return ImTextureID			A DescriptorSet as ImGui identifier for textures
		 */
		ImTextureID get_or_create_texture_descriptor(const avk::image_sampler_t& aImageSampler, avk::layout::image_layout aImageLayout);

		void set_use_fence_for_font_upload() {
			mUsingSemaphoreInsteadOfFenceForFontUpload = false;
		}

		void set_use_semaphore_for_font_upload() {
			mUsingSemaphoreInsteadOfFenceForFontUpload = true;
		}

		/** Configure imgui_manager which font to use. 
		 *  @param aPathToTtfFont	Path to a custom TTF font file to be used, or empty for using ImGui's bundled default font.
		 */
        void set_custom_font(std::string aPathToTtfFont = {}) {
			mCustomTtfFont = std::move(aPathToTtfFont);
		}

        /** Indicates whether or not ImGui wants to occupy the mouse.
         *  This could be because the mouse is over a window, or currently dragging
         *	some ImGui control.
         *  \return True if ImGui wants to occupy the mouse in the current frame.
         */
        bool want_to_occupy_mouse() const {
			return mOccupyMouse;
		}

		/** Indicates whether or not ImGui wants to START occupying the mouse.
		 *  \return True if ImGui wants to occupy the mouse in the current frame, but didn't in the previous frame.
		 */
		bool begin_wanting_to_occupy_mouse() const {
			return mOccupyMouse && !mOccupyMouseLastFrame;
		}

		/** Indicates whether or not ImGui ENDS its desire for occupying the mouse.
		 *  \return True if ImGui wanted to occupy the mouse in the previous frame, but doesn't want anymore.
		 */
		bool end_wanting_to_occupy_mouse() const {
			return !mOccupyMouse && mOccupyMouseLastFrame;
		}

		/** Gets the image layout that imgui_manager assumes the color attachment to be in when it starts
		 *  to render into it.
		 *	The default value is: avk::layout::color_attachment_optimal
		 */
		layout::image_layout previous_image_layout() const {
			return mIncomingLayout;
		}

		/** Sets the image layout that imgui_manager shall expect the color attachment to be in
		 *  before starting to render into it.
		 *	Note: This change might only have effect BEFORE imgui_manager has been initialized,
		 *	      i.e., before its ::initialize() method has been invoked by the framework for the
		 *		  first time. => Ensure to configure this value before starting the composition!
		 */
		void set_previous_image_layout(layout::image_layout aIncomingLayout) {
			mIncomingLayout = aIncomingLayout;
		}
	private:
		void upload_fonts();
		void construct_render_pass();

		avk::queue* mQueue;
		avk::descriptor_pool mDescriptorPool;
		avk::command_pool_t* mCommandPool; // TODO: This should probably be an avk::resoruce_reference<avk::command_pool_t>
		std::vector<avk::command_buffer> mCommandBuffers; // <-- one for each concurrent frame
		std::vector<avk::semaphore> mRenderFinishedSemaphores; // <-- one for each concurrent frame

		avk::renderpass mRenderpass;
		
		// Descriptor cache for imgui texture
		avk::descriptor_cache mImTextureDescriptorCache;

		// this render pass is used to reset the attachment if no invokee has previously written on it
		// TODO is there a better solution(?)
		avk::renderpass mClearRenderpass;
		std::vector<avk::unique_function<void()>> mCallback;
		int mExecutionOrder;
		int mMouseCursorPreviousValue;
		bool mUserInteractionEnabled;
		bool mAlreadyRendered;
		bool mUsingSemaphoreInsteadOfFenceForFontUpload;
		bool mOccupyMouse = false;
		bool mOccupyMouseLastFrame = false;

		// customization
		std::function<void(float)> mAlterSettingsBeforeInitialization = {};
		std::string mCustomTtfFont = {};
		layout::image_layout mIncomingLayout;
	};

}
