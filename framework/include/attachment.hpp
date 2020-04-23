#pragma once

namespace cgb
{
	struct attachment;
	
	namespace att
	{
		enum struct on_load
		{
			dont_care,
			clear,
			load
		};

		enum struct on_store
		{
			dont_care,
			store,
			store_in_presentable_format
		};

		struct usage_type
		{
			static usage_type create_unused()			{ return { false, false, false, false,  -1,  -1, false, -1 }; }
			static usage_type create_input(int loc)		{ return { true , false, false, false, loc,  -1, false, -1 }; }
			static usage_type create_color(int loc)		{ return { false, true , false, false,  -1, loc, false, -1 }; }
			static usage_type create_depth_stencil()	{ return { false, false, true , false,  -1,  -1, false, -1 }; }
			static usage_type create_preserve()			{ return { false, false, false, true ,  -1,  -1, false, -1 }; }
			
			bool as_unused() const { return !(mInput || mColor || mDepthStencil || mPreserve); }
			bool as_input() const { return mInput; }
			bool as_color() const { return mColor; }
			bool as_depth_stencil() const { return mDepthStencil; }
			bool as_preserve() const { return mPreserve; }

			bool has_input_location() const { return -1 != mInputLocation; }
			bool has_color_location() const { return -1 != mColorLocation; }
			auto input_location() const { return mInputLocation; }
			auto color_location() const { return mColorLocation; }
			bool has_resolve() const { return mResolve; }
			auto resolve_target_index() const { return mResolveAttachmentIndex; }
			
			bool mInput;
			bool mColor;
			bool mDepthStencil;
			bool mPreserve;
			int mInputLocation;
			int mColorLocation;
			bool mResolve;
			int mResolveAttachmentIndex;
		};

		class usage_desc
		{
			friend struct ::cgb::attachment;
			friend class ::cgb::renderpass_t;
		public:
			usage_desc(usage_desc&&) noexcept = default;
			usage_desc(const usage_desc&) = default;
			//usage_desc(const usage_desc* ud) noexcept : mDescriptions{ud->mDescriptions} {};
			usage_desc& operator=(usage_desc&&) noexcept = default;
			usage_desc& operator=(const usage_desc&) = default;
			//usage_desc& operator=(const usage_desc* ud) noexcept { mDescriptions = ud->mDescriptions; return *this; };
			virtual ~usage_desc() {}
			
			usage_desc& unused()							{ mDescriptions.emplace_back(usage_type::create_unused()); return *this; }
			usage_desc& resolve_receiver()					{ return unused(); }
			usage_desc& input(int location)					{ mDescriptions.emplace_back(usage_type::create_input(location)); return *this; }
			usage_desc& color(int location)					{ mDescriptions.emplace_back(usage_type::create_color(location)); return *this; }
			usage_desc& depth_stencil()						{ mDescriptions.emplace_back(usage_type::create_depth_stencil()); return *this; }
			usage_desc& preserve()							{ mDescriptions.emplace_back(usage_type::create_preserve()); return *this; }

			usage_desc* operator->()						{ return this; }
			usage_desc& operator+(usage_desc additionalUsages);

		protected:
			bool contains_unused() const		{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const usage_type& u) { return u.as_unused(); }) != mDescriptions.end(); }
			bool contains_input() const			{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const usage_type& u) { return u.as_input(); }) != mDescriptions.end(); }
			bool contains_color() const			{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const usage_type& u) { return u.as_color(); }) != mDescriptions.end(); }
			bool contains_resolve() const		{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const usage_type& u) { return u.has_resolve(); }) != mDescriptions.end(); }
			bool contains_depth_stencil() const	{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const usage_type& u) { return u.as_depth_stencil(); }) != mDescriptions.end(); }
			bool contains_preserve() const		{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const usage_type& u) { return u.as_preserve(); }) != mDescriptions.end(); }

			usage_type first_color_depth_input_usage() const
			{
				auto n = mDescriptions.size();
				for (size_t i = 0; i < n; ++i) {
					if (mDescriptions[i].mColor || mDescriptions[i].mDepthStencil || mDescriptions[i].mInput) {
						return mDescriptions[i];
					}
				}
				return mDescriptions[0];
			}

			usage_type last_color_depth_input_usage() const
			{
				auto n = mDescriptions.size();
				for (size_t i = n; i > 0; --i) {
					if (mDescriptions[i-1].mColor || mDescriptions[i-1].mDepthStencil || mDescriptions[i-1].mInput) {
						return mDescriptions[i-1];
					}
				}
				return mDescriptions[n-1];
			}

			auto num_subpasses() const { return mDescriptions.size(); }
			auto get_subpass_usage(size_t subpassId) const { return mDescriptions[subpassId]; }
			//auto is_to_be_resolved_after_subpass(size_t subpassId) const { return mDescriptions[subpassId].has_resolve(); }
			//auto get_resolve_target_index(size_t subpassId) const { return mDescriptions[subpassId].mResolveAttachmentIndex; }
			//auto has_input_location_at_subpass(size_t subpassId) const { return mDescriptions[subpassId].as_input() && mDescriptions[subpassId].mInputLocation != -1; }
			//auto input_location_at_subpass(size_t subpassId) const { return mDescriptions[subpassId].mInputLocation; }
			//auto has_color_location_at_subpass(size_t subpassId) const { return mDescriptions[subpassId].as_color() && mDescriptions[subpassId].mColorLocation != -1; }
			//auto color_location_at_subpass(size_t subpassId) const { return mDescriptions[subpassId].mColorLocation; }

		protected:
			usage_desc() = default;
			std::vector<usage_type> mDescriptions;
		};

		class unused : public usage_desc
		{
		public:
			explicit unused()											{ mDescriptions.emplace_back(usage_type::create_unused()); }
			unused(unused&&) noexcept = default;
			unused(const unused&) = default;
			unused& operator=(unused&&) noexcept = default;
			unused& operator=(const unused&) = default;
		};

		using resolve_receiver = unused;
		
		class input : public usage_desc
		{
		public:
			explicit input(int location)								{ mDescriptions.emplace_back(usage_type::create_input(location)); }
			input(input&&) noexcept = default;
			input(const input&) = default;
			input& operator=(input&&) noexcept = default;
			input& operator=(const input&) = default;
		};
		
		class color : public usage_desc
		{
		public:
			explicit color(int location)								{ mDescriptions.emplace_back(usage_type::create_color(location)); }
			color(color&&) noexcept = default;
			color(const color&) = default;
			color& operator=(color&&) noexcept = default;
			color& operator=(const color&) = default;
		};

		/** Resolve this attachment and store the resolved results to another attachment at the specified index. */
		class resolve_to : public usage_desc
		{
		public:
			/**	Indicate that this attachment shall be resolved.
			 *	@param targetLocation	The index of the attachment where to resolve this attachment into.
			 */
			explicit resolve_to(int targetLocation)						{ auto& r = mDescriptions.emplace_back(usage_type::create_unused()); r.mResolve = true; r.mResolveAttachmentIndex = targetLocation; }
			resolve_to(resolve_to&&) noexcept = default;										// ^ usage_type not applicable here, but actually it *is* unused.
			resolve_to(const resolve_to&) = default;
			resolve_to& operator=(resolve_to&&) noexcept = default;
			resolve_to& operator=(const resolve_to&) = default;
		};
		
		class depth_stencil : public usage_desc
		{
		public:
			explicit depth_stencil(int location = 0)					{ mDescriptions.emplace_back(usage_type::create_depth_stencil()); }
			depth_stencil(depth_stencil&&) noexcept = default;
			depth_stencil(const depth_stencil&) = default;
			depth_stencil& operator=(depth_stencil&&) noexcept = default;
			depth_stencil& operator=(const depth_stencil&) = default;
		};
		
		class preserve : public usage_desc
		{
		public:
			explicit preserve()											{ mDescriptions.emplace_back(usage_type::create_preserve()); }
			preserve(preserve&&) noexcept = default;
			preserve(const preserve&) = default;
			preserve& operator=(preserve&&) noexcept = default;
			preserve& operator=(const preserve&) = default;
		};
		
	}

	/** Describes an attachment to a framebuffer or a renderpass.
	 *	It can describe color attachments as well as depth/stencil attachments
	 *	and holds some additional config parameters for these attachments.
	 */
	struct attachment
	{
		static attachment declare(std::tuple<image_format, int> aFormatAndSamples, att::on_load aLoadOp, att::usage_desc aUsageInSubpasses, att::on_store aStoreOp);
		static attachment declare(image_format aFormat, att::on_load aLoadOp, att::usage_desc aUsageInSubpasses, att::on_store aStoreOp);
		static attachment declare_for(const image_view_t& aImageView, att::on_load aLoadOp, att::usage_desc aUsageInSubpasses, att::on_store aStoreOp);

		attachment& set_clear_color(glm::vec4 aColor);
		attachment& set_depth_clear_value(float aDepthClear);
		attachment& set_stencil_clear_value(uint32_t aStencilClear);
		
		attachment& set_load_operation(att::on_load aLoadOp);
		attachment& set_store_operation(att::on_store aStoreOp);
		attachment& load_contents();
		attachment& clear_contents();
		attachment& store_contents();
		attachment& set_stencil_load_operation(att::on_load aLoadOp);
		attachment& set_stencil_store_operation(att::on_store aStoreOp);
		attachment& load_stencil_contents();
		attachment& clear_stencil_contents();
		attachment& store_stencil_contents();

		attachment& set_image_usage_hint(cgb::image_usage aImageUsageBeforeAndAfter);
		attachment& set_image_usage_hints(cgb::image_usage aImageUsageBefore, cgb::image_usage aImageUsageAfter);
		
		/** The color/depth/stencil format of the attachment */
		auto format() const { return mFormat; }
		
		auto shall_be_presentable() const { return att::on_store::store_in_presentable_format == mStoreOperation; }

		auto get_first_color_depth_input() const { return mSubpassUsages.first_color_depth_input_usage(); }

		auto get_last_color_depth_input() const { return mSubpassUsages.last_color_depth_input_usage(); }
		
		auto is_used_as_depth_stencil_attachment() const { return mSubpassUsages.contains_depth_stencil(); }

		auto is_used_as_color_attachment() const { return mSubpassUsages.contains_color(); }
		
		auto is_used_as_input_attachment() const { return mSubpassUsages.contains_input(); }

		/** True if the sample count is greater than 1 */
		bool is_multisampled() const { return mSampleCount > 1; }
		/** The sample count for this attachment. */
		auto sample_count() const { return mSampleCount; }
		/** True if a multisample resolve pass shall be set up. */
		auto is_to_be_resolved() const { return mSubpassUsages.contains_resolve(); }

		/** Returns the stencil load operation */
		auto get_stencil_load_op() const { return mStencilLoadOperation.value_or(mLoadOperation); }
		/** Returns the stencil store operation */
		auto get_stencil_store_op() const { return mStencilStoreOperation.value_or(mStoreOperation); }

		auto clear_color() const { return mColorClearValue; }
		auto depth_clear_value() const { return mDepthClearValue; }
		auto stencil_clear_value() const { return mStencilClearValue; }

		image_format mFormat;
		int mSampleCount;
		att::on_load mLoadOperation;
		att::on_store mStoreOperation;
		std::optional<att::on_load> mStencilLoadOperation;
		std::optional<att::on_store> mStencilStoreOperation;
		att::usage_desc mSubpassUsages;
		glm::vec4 mColorClearValue;
		float mDepthClearValue;
		uint32_t mStencilClearValue;
		std::optional<image_usage> mImageUsageHintBefore;
		std::optional<image_usage> mImageUsageHintAfter;
	};
}
