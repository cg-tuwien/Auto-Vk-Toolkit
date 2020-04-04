#pragma once

namespace cgb
{
	namespace cfg
	{
		enum struct attachment_load_operation
		{
			dont_care,
			clear,
			load
		};

		enum struct attachment_store_operation
		{
			dont_care,
			store,
			store_in_presentable_format
		};

		struct sample_count
		{
			sample_count(int numSamples) : mNumSamples{ numSamples } {}
			sample_count(sample_count&&) noexcept = default;
			sample_count(const sample_count&) noexcept = default;
			sample_count& operator=(sample_count&&) noexcept = default;
			sample_count& operator=(const sample_count&) noexcept = default;
			
			int mNumSamples;
		};
	}

	struct attachment;
	
	namespace used_as
	{
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
			
			usage_desc& unused(int location = -1)			{ mDescriptions.emplace_back(usage_type::unused,		false, location); return *this; }
			usage_desc& input(int location = -1)			{ mDescriptions.emplace_back(usage_type::input,			false, location); return *this; }
			usage_desc& color(int location = -1)			{ mDescriptions.emplace_back(usage_type::color,			false, location); return *this; }
			usage_desc& depth_stencil(int location = -1)	{ mDescriptions.emplace_back(usage_type::depth_stencil,	false, location); return *this; }
			usage_desc& preserve(int location = -1)			{ mDescriptions.emplace_back(usage_type::preserve,		false, location); return *this; }

			usage_desc* operator->()						{ return this; }
			usage_desc& operator+(usage_desc& resolveAndMore);

		protected:
			bool contains_unused() const		{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const auto& tpl) { return std::get<usage_type>(tpl) == usage_type::unused;			}) != mDescriptions.end(); }
			bool contains_input() const			{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const auto& tpl) { return std::get<usage_type>(tpl) == usage_type::input;			}) != mDescriptions.end(); }
			bool contains_color() const			{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const auto& tpl) { return std::get<usage_type>(tpl) == usage_type::color;			}) != mDescriptions.end(); }
			bool contains_resolve() const		{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const auto& tpl) { return std::get<bool>(tpl) == true;								}) != mDescriptions.end(); }
			bool contains_depth_stencil() const	{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const auto& tpl) { return std::get<usage_type>(tpl) == usage_type::depth_stencil;	}) != mDescriptions.end(); }
			bool contains_preserve() const		{ return std::find_if(mDescriptions.begin(), mDescriptions.end(), [](const auto& tpl) { return std::get<usage_type>(tpl) == usage_type::preserve;		}) != mDescriptions.end(); }

			auto num_subpasses() const { return mDescriptions.size(); }
			auto get_subpass_usage(size_t subpassId) const { return std::get<usage_type>(mDescriptions[subpassId]); }
			auto is_to_be_resolved_after_subpass(size_t subpassId) const { return std::get<bool>(mDescriptions[subpassId]); }
			auto has_layout_at_subpass(size_t subpassId) const { return std::get<int>(mDescriptions[subpassId]) != -1; }
			auto layout_at_subpass(size_t subpassId) const { return std::get<int>(mDescriptions[subpassId]); }

		protected:
			usage_desc() = default;
			std::vector<std::tuple<usage_type, bool, int>> mDescriptions;
		};

		class unused : public usage_desc
		{
		public:
			unused(int location = -1)						{ mDescriptions.emplace_back(usage_type::unused,		false, location); }
			unused(unused&&) noexcept = default;
			unused(const unused&) = default;
			unused& operator=(unused&&) noexcept = default;
			unused& operator=(const unused&) = default;
		};
		
		class input : public usage_desc
		{
		public:
			input(int location = -1)						{ mDescriptions.emplace_back(usage_type::input,			false, location); }
			input(input&&) noexcept = default;
			input(const input&) = default;
			input& operator=(input&&) noexcept = default;
			input& operator=(const input&) = default;
		};
		
		class color : public usage_desc
		{
		public:
			color(int location = -1)						{ mDescriptions.emplace_back(usage_type::color,			false, location); }
			color(color&&) noexcept = default;
			color(const color&) = default;
			color& operator=(color&&) noexcept = default;
			color& operator=(const color&) = default;
		};
		
		class resolve : public usage_desc
		{
		public:
			resolve(bool doResolve = true)					{ mDescriptions.emplace_back(usage_type::unused,		doResolve, -1); }
			resolve(resolve&&) noexcept = default;										// ^ usage_type not applicable here
			resolve(const resolve&) = default;
			resolve& operator=(resolve&&) noexcept = default;
			resolve& operator=(const resolve&) = default;
		};
		
		class depth_stencil : public usage_desc
		{
		public:
			depth_stencil(int location = -1)				{ mDescriptions.emplace_back(usage_type::depth_stencil,	false, location); }
			depth_stencil(depth_stencil&&) noexcept = default;
			depth_stencil(const depth_stencil&) = default;
			depth_stencil& operator=(depth_stencil&&) noexcept = default;
			depth_stencil& operator=(const depth_stencil&) = default;
		};
		
		class preserve : public usage_desc
		{
		public:
			preserve(int location = -1)						{ mDescriptions.emplace_back(usage_type::preserve,		false, location); }
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
		static attachment define(std::tuple<image_format, cfg::sample_count> aFormatAndSamples, cfg::attachment_load_operation aLoadOp, used_as::usage_desc aUsageInSubpasses, cfg::attachment_store_operation aStoreOp);
		static attachment define(image_format aFormat, cfg::attachment_load_operation aLoadOp, used_as::usage_desc aUsageInSubpasses, cfg::attachment_store_operation aStoreOp);
		static attachment define_for(const image_view_t& aImageView, cfg::attachment_load_operation aLoadOp, used_as::usage_desc aUsageInSubpasses, cfg::attachment_store_operation aStoreOp);

		attachment& set_load_operation(cfg::attachment_load_operation aLoadOp);
		attachment& set_store_operation(cfg::attachment_store_operation aStoreOp);
		attachment& load_contents();
		attachment& clear_contents();
		attachment& store_contents();
		attachment& set_stencil_load_operation(cfg::attachment_load_operation aLoadOp);
		attachment& set_stencil_store_operation(cfg::attachment_store_operation aStoreOp);
		attachment& load_stencil_contents();
		attachment& clear_stencil_contents();
		attachment& store_stencil_contents();
		
		/** The color/depth/stencil format of the attachment */
		auto format() const { return mFormat; }
		/** True if this attachment shall be, finally, presented on screen. */
		auto is_presentable() const { return cfg::attachment_store_operation::store_in_presentable_format == mStoreOperation; }
		/** True if this attachment is used as depth/stencil attachment in the subpasses. */
		auto is_depth_stencil_attachment() const { return mSubpassUsages.contains_depth_stencil(); }
		/** True if this attachment is used as color attachment in the subpasses. */
		auto is_color_attachment() const { return mSubpassUsages.contains_color(); }
		/** True if this attachment is only used as input attachment in the subpasses, but not as color or depth attachment */
		auto is_pure_input_attachment() const { return mSubpassUsages.contains_input() && !is_depth_stencil_attachment() && !is_color_attachment(); }
		/** True fi the sample count is greater than 1 */
		bool is_multisampled() const { return mSampleCount > 1; }
		/** The sample count for this attachment. */
		auto sample_count() const { return mSampleCount; }
		/** True if a multisample resolve pass shall be set up. */
		auto is_to_be_resolved() const { return mSubpassUsages.contains_resolve(); }

		/** Returns the stencil load operation */
		auto get_stencil_load_op() const { return mStencilLoadOperation.value_or(mLoadOperation); }
		/** Returns the stencil store operation */
		auto get_stencil_store_op() const { return mStencilStoreOperation.value_or(mStoreOperation); }

		image_format mFormat;
		int mSampleCount;
		cfg::attachment_load_operation mLoadOperation;
		cfg::attachment_store_operation mStoreOperation;
		std::optional<cfg::attachment_load_operation> mStencilLoadOperation;
		std::optional<cfg::attachment_store_operation> mStencilStoreOperation;
		used_as::usage_desc mSubpassUsages;
	};
}
