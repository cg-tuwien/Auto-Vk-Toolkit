#pragma once
#include <gvk.hpp>
#include <imgui.h>

namespace gvk
{
	class imgui_manager : public invokee
	{
	public:
		/**	Create an ImGui manager element.
		 *	@param		aName				You *can* give it a name, but you can also leave it at the default name "imgui".
		 *	@param		aExecutionOrder		UI should probably draw after most/all of the other invokees.
		 *									Therefore, use a high execution order. Default value is 100000.
		 */
		imgui_manager(avk::queue& aQueueToSubmitTo, std::string aName = "imgui", std::optional<avk::renderpass> aRenderpassToUse = {}, int aExecutionOrder = 100000)
			: invokee(std::move(aName))
			, mQueue { &aQueueToSubmitTo }
			, mExecutionOrder{ aExecutionOrder }
			, mUserInteractionEnabled{ true }
		{
			if (aRenderpassToUse.has_value()) {
				mRenderpass = std::move(aRenderpassToUse.value());
			}
		}

		/** ImGui should run very late -> hence, the default value of 100000 in the constructor. */
		int execution_order() const override { return mExecutionOrder; }

		void initialize() override;

		void update() override;

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
		 *	@param[in] aImageSampler		An image sampler, that shall be rendered with ImGui
		 *  @param[out] ImTextureID			A DescriptorSet as ImGui identifier for textures
		 */
		ImTextureID get_or_create_texture_descriptor(avk::resource_reference<avk::image_sampler_t> aImageSampler);

	private:
		void upload_fonts();
		void construct_render_pass();

		avk::queue* mQueue;
		avk::descriptor_pool mDescriptorPool;
		avk::command_pool mCommandPool;
		std::optional<avk::renderpass> mRenderpass;

		// Descriptor cache for imgui texture
		avk::descriptor_cache mImTextureDescriptorCache;

		// this render pass is used to reset the attachment if no invokee has previously written on it
		// TODO is there a better solution(?)
		std::optional<avk::renderpass> mClearRenderpass;
		std::vector<avk::unique_function<void()>> mCallback;
		int mExecutionOrder;
		int mMouseCursorPreviousValue;
		bool mUserInteractionEnabled;
	};

}
