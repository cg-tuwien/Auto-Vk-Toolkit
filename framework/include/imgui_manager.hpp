#pragma once

namespace cgb
{
	class imgui_manager : public cg_element
	{
	public:
		/**	Create an ImGui manager element.
		 *	@param		aName				You *can* give it a name, but you can also leave it at the default name "imgui".
		 *	@param		aExecutionOrder		UI should probably draw after most/all of the other cg_elements.
		 *									Therefore, use a high execution order. Default value is 100000.
		 */
		imgui_manager(std::string aName = "imgui", std::optional<renderpass> aRenderpassToUse = {}, int aExecutionOrder = 100000)
			: cg_element(std::move(aName))
			, mExecutionOrder(aExecutionOrder)
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
		bool is_user_interaction_enabled() { return mUserInteractionEnabled; }

	private:
		int mExecutionOrder;
		std::shared_ptr<cgb::descriptor_pool> mDescriptorPool;
		std::optional<renderpass> mRenderpass;
		int mMouseCursorPreviousValue;
		std::vector<unique_function<void()>> mCallback;
		bool mUserInteractionEnabled;
	};
}
