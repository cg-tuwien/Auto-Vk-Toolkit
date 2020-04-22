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

		enum struct usage_type
		{
			unused,
			input,
			color,
			depth_stencil,
			preserve
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
			
			usage_desc& unused()							{ mDescriptions.emplace_back(usage_type::unused,		std::optional<int>{}, -1); return *this; }
			usage_desc& resolve_receiver()					{ return unused(); }
			usage_desc& input(int location)					{ mDescriptions.emplace_back(usage_type::input,			std::optional<int>{}, location); return *this; }
			usage_desc& color(int location)					{ mDescriptions.emplace_back(usage_type::color,			std::optional<int>{}, location); return *this; }
			usage_desc& depth_stencil(int location = 0)		{ mDescriptions.emplace_back(usage_type::depth_stencil,	std::optional<int>{}, location); return *this; }
			usage_desc& preserve()							{ mDescriptions.emplace_back(usage_type::preserve,		std::optional<int>{}, -1); return *this; }

			usage_desc* operator->()						{ return this; }
			usage_desc& operator+(usage_desc resolveAndMore);

		protected:
			bool contains_unused() const		{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const auto& tpl) { return std::get<usage_type>(tpl) == usage_type::unused;			}) != mDescriptions.end(); }
			bool contains_input() const			{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const auto& tpl) { return std::get<usage_type>(tpl) == usage_type::input;			}) != mDescriptions.end(); }
			bool contains_color() const			{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const auto& tpl) { return std::get<usage_type>(tpl) == usage_type::color;			}) != mDescriptions.end(); }
			bool contains_resolve() const		{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const auto& tpl) { return std::get<std::optional<int>>(tpl).has_value();			}) != mDescriptions.end(); }
			bool contains_depth_stencil() const	{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const auto& tpl) { return std::get<usage_type>(tpl) == usage_type::depth_stencil;	}) != mDescriptions.end(); }
			bool contains_preserve() const		{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const auto& tpl) { return std::get<usage_type>(tpl) == usage_type::preserve;		}) != mDescriptions.end(); }

			usage_type first_color_depth_input_usage() const
			{
				auto n = mDescriptions.size();
				usage_type firstUt = usage_type::unused;
				for (size_t i = 0; i < n; ++i) {
					auto ut = std::get<usage_type>(mDescriptions[i]);
					if (usage_type::color == ut || usage_type::depth_stencil == ut || usage_type::input == ut) {
						return ut;
					}
					if (i > 0) { continue; }
					firstUt = ut;
				}
				return firstUt;
			}

			usage_type last_color_depth_input_usage() const
			{
				auto n = mDescriptions.size();
				usage_type lastUt = usage_type::unused;
				for (size_t i = n; i > 0; --i) {
					auto ut = std::get<usage_type>(mDescriptions[i - 1]);
					if (usage_type::color == ut || usage_type::depth_stencil == ut || usage_type::input == ut) {
						return ut;
					}
					if (i < n) { continue; }
					lastUt = ut;
				}
				return lastUt;
			}

			auto num_subpasses() const { return mDescriptions.size(); }
			auto get_subpass_usage(size_t subpassId) const { return std::get<usage_type>(mDescriptions[subpassId]); }
			auto is_to_be_resolved_after_subpass(size_t subpassId) const { return std::get<std::optional<int>>(mDescriptions[subpassId]).has_value(); }
			auto get_resolve_target_index(size_t subpassId) const { return std::get<std::optional<int>>(mDescriptions[subpassId]).value(); }
			auto has_layout_at_subpass(size_t subpassId) const { return std::get<int>(mDescriptions[subpassId]) != -1; }
			auto layout_at_subpass(size_t subpassId) const { return std::get<int>(mDescriptions[subpassId]); }

		protected:
			usage_desc() = default;
			std::vector<std::tuple<usage_type, std::optional<int>, int>> mDescriptions;
		};

		class unused : public usage_desc
		{
		public:
			explicit unused()											{ mDescriptions.emplace_back(usage_type::unused,		std::optional<int>{}, -1); }
			unused(unused&&) noexcept = default;
			unused(const unused&) = default;
			unused& operator=(unused&&) noexcept = default;
			unused& operator=(const unused&) = default;
		};

		using resolve_receiver = unused;
		
		class input : public usage_desc
		{
		public:
			explicit input(int location)								{ mDescriptions.emplace_back(usage_type::input,			std::optional<int>{}, location); }
			input(input&&) noexcept = default;
			input(const input&) = default;
			input& operator=(input&&) noexcept = default;
			input& operator=(const input&) = default;
		};
		
		class color : public usage_desc
		{
		public:
			explicit color(int location)								{ mDescriptions.emplace_back(usage_type::color,			std::optional<int>{}, location); }
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
			explicit resolve_to(int targetLocation)						{ mDescriptions.emplace_back(usage_type::unused,		std::optional<int>{ targetLocation }, -1); }
			resolve_to(resolve_to&&) noexcept = default;										// ^ usage_type not applicable here, but actually it *is* unused.
			resolve_to(const resolve_to&) = default;
			resolve_to& operator=(resolve_to&&) noexcept = default;
			resolve_to& operator=(const resolve_to&) = default;
		};
		
		class depth_stencil : public usage_desc
		{
		public:
			explicit depth_stencil(int location = 0)					{ mDescriptions.emplace_back(usage_type::depth_stencil,	std::optional<int>{}, location); }
			depth_stencil(depth_stencil&&) noexcept = default;
			depth_stencil(const depth_stencil&) = default;
			depth_stencil& operator=(depth_stencil&&) noexcept = default;
			depth_stencil& operator=(const depth_stencil&) = default;
		};
		
		class preserve : public usage_desc
		{
		public:
			explicit preserve()											{ mDescriptions.emplace_back(usage_type::preserve,		std::optional<int>{}, -1); }
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

		auto get_first_usage_type() const { return mSubpassUsages.first_color_depth_input_usage(); }

		auto get_last_usage_type() const { return mSubpassUsages.last_color_depth_input_usage(); }
		
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
