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
			store
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

	namespace att
	{
		enum struct usage_type
		{
			unused,
			input,
			color,
			resolve,
			depth_stencil,
			preserve
		};
		
		class usage_desc
		{
		public:
			usage_desc(usage_desc&&) noexcept = default;
			usage_desc(const usage_desc&) noexcept = default;
			usage_desc(const usage_desc* ud) noexcept : mDescriptions{ud->mDescriptions} {};
			usage_desc& operator=(usage_desc&&) noexcept = default;
			usage_desc& operator=(const usage_desc&) noexcept = default;
			usage_desc& operator=(const usage_desc* ud) noexcept { mDescriptions = ud->mDescriptions; return *this; };
			
			usage_desc* unused(int location = -1)			{ mDescriptions.emplace_back(usage_type::unused,		location); return this; }
			usage_desc* input(int location = -1)			{ mDescriptions.emplace_back(usage_type::input,			location); return this; }
			usage_desc* color(int location = -1)			{ mDescriptions.emplace_back(usage_type::color,			location); return this; }
			usage_desc* resolve(int location = -1)			{ mDescriptions.emplace_back(usage_type::resolve,		location); return this; }
			usage_desc* depth_stencil(int location = -1)	{ mDescriptions.emplace_back(usage_type::depth_stencil,	location); return this; }
			usage_desc* preserve(int location = -1)			{ mDescriptions.emplace_back(usage_type::preserve,		location); return this; }
		protected:
			usage_desc() = default;
			std::vector<std::tuple<usage_type, int>> mDescriptions;
		};

		class unused : public usage_desc
		{
		public:
			unused(int location = -1)						{ mDescriptions.emplace_back(usage_type::unused,		location); }
			unused(unused&&) noexcept = default;
			unused(const unused&) noexcept = default;
			unused& operator=(unused&&) noexcept = default;
			unused& operator=(const unused&) noexcept = default;
		};
		
		class input : public usage_desc
		{
		public:
			input(int location = -1)						{ mDescriptions.emplace_back(usage_type::input,			location); }
			input(input&&) noexcept = default;
			input(const input&) noexcept = default;
			input& operator=(input&&) noexcept = default;
			input& operator=(const input&) noexcept = default;
		};
		
		class color : public usage_desc
		{
		public:
			color(int location = -1)						{ mDescriptions.emplace_back(usage_type::color,			location); }
			color(color&&) noexcept = default;
			color(const color&) noexcept = default;
			color& operator=(color&&) noexcept = default;
			color& operator=(const color&) noexcept = default;
		};
		
		class resolve : public usage_desc
		{
		public:
			resolve(int location = -1)						{ mDescriptions.emplace_back(usage_type::resolve,		location); }
			resolve(resolve&&) noexcept = default;
			resolve(const resolve&) noexcept = default;
			resolve& operator=(resolve&&) noexcept = default;
			resolve& operator=(const resolve&) noexcept = default;
		};
		
		class depth_stencil : public usage_desc
		{
		public:
			depth_stencil(int location = -1)				{ mDescriptions.emplace_back(usage_type::depth_stencil,	location); }
			depth_stencil(depth_stencil&&) noexcept = default;
			depth_stencil(const depth_stencil&) noexcept = default;
			depth_stencil& operator=(depth_stencil&&) noexcept = default;
			depth_stencil& operator=(const depth_stencil&) noexcept = default;
		};
		
		class preserve : public usage_desc
		{
		public:
			preserve(int location = -1)						{ mDescriptions.emplace_back(usage_type::preserve,		location); }
			preserve(preserve&&) noexcept = default;
			preserve(const preserve&) noexcept = default;
			preserve& operator=(preserve&&) noexcept = default;
			preserve& operator=(const preserve&) noexcept = default;
		};
		
	}

	/** Describes an attachment to a framebuffer or a renderpass.
	 *	It can describe color attachments as well as depth/stencil attachments
	 *	and holds some additional config parameters for these attachments.
	 */
	struct attachment
	{
		/** Create a new color attachment, with the following default settings:
		 *	 - is presentable
		 *	 - don't care on load
		 *	 - don't care on store
		 *	@param	pFormat			The color format of the new attachment
		 *	@param	pLocation		(Optional) At which layout location shall this color attachment appear.
		 */
		static attachment create_color(image_format pFormat, image_usage pImageUsage = image_usage::color_attachment, std::optional<uint32_t> pLocation = {});

		/** Create a new depth/stencil attachment, with the following default settings:
		 *	 - don't care on load
		 *	 - don't care on store
		 *	@param	pFormat		(Optional) The depth format of the new attachment
		 *	@param	pLocation	(Optional) At which layout location shall this depth/stencil attachment appear.
		 */
		static attachment create_depth(std::optional<image_format> pFormat = {}, image_usage pImageUsage = image_usage::depth_stencil_attachment, std::optional<uint32_t> pLocation = {});

		/** Create a new depth/stencil attachment, with the following default settings:
		 *	 - don't care on load
		 *	 - don't care on store
		 *	@param	pFormat		(Optional) The depth/stencil format of the new attachment
		 *	@param	pLocation	(Optional) At which layout location shall this depth/stencil attachment appear.
		 */
		static attachment create_depth_stencil(std::optional<image_format> pFormat = {}, image_usage pImageUsage = image_usage::depth_stencil_attachment, std::optional<uint32_t> pLocation = {});

		/** Create a new color attachment for use as shader input, with the following default settings:
		 *	 - don't care on load
		 *	 - don't care on store
		 *	@param	pFormat		The color format of the new attachment
		 *	@param	pLocation	(Optional) At which layout location shall this color attachment appear.
		 */
		static attachment create_shader_input(image_format pFormat, image_usage pImageUsage = image_usage::read_only_sampled_image, std::optional<uint32_t> pLocation = {});

		/** Create a new multisampled color attachment, with the following default settings:
		 *	 - is presentable
		 *	 - don't care on load
		 *	 - don't care on store
		 *	@param	pFormat					The color format of the new attachment
		 *	@param	pSampleCount			The number of samples per fragment. A value of 1 means that multisampling is disabled.
		 *	@param	pResolveMultisamples	If set to true, a multisample resolve pass will be set up.
		 *	@param	pLocation				(Optional) At which layout location shall this color attachment appear.
		 */
		static attachment create_color_multisampled(image_format pFormat, int pSampleCount, bool pResolveMultisamples, image_usage pImageUsage = image_usage::color_attachment, std::optional<uint32_t> pLocation = {});

		/** Create a new multisampled depth/stencil attachment
		 *	 - don't care on load
		 *	 - don't care on store
		 *	@param	pFormat					The depth/stencil format of the new attachment
		 *	@param	pSampleCount			The number of samples per fragment. A value of 1 means that multisampling is disabled.
		 *	@param	pResolveMultisamples	If set to true, a multisample resolve pass will be set up.
		 *	@param	pLocation				(Optional) At which layout location shall this depth/stencil attachment appear.
		 */
		static attachment create_depth_multisampled(image_format pFormat, int pSampleCount, bool pResolveMultisamples, image_usage pImageUsage = image_usage::depth_stencil_attachment, std::optional<uint32_t> pLocation = {});

		/** Create a new multisampled color attachment for use as shader input
		 *	 - don't care on load
		 *	 - don't care on store
		 *	@param	pFormat					The color format of the new attachment
		 *	@param	pSampleCount			The number of samples per fragment. A value of 1 means that multisampling is disabled.
		 *	@param	pResolveMultisamples	If set to true, a multisample resolve pass will be set up.
		 *	@param	pLocation				(Optional) At which layout location shall this color attachment appear.
		 */
		static attachment create_shader_input_multisampled(image_format pFormat, int pSampleCount, bool pResolveMultisamples, image_usage pImageUsage = image_usage::depth_stencil_attachment, std::optional<uint32_t> pLocation = {});

		static attachment create_for(const image_view_t& _ImageView, std::optional<uint32_t> pLocation = {});

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
		/** True if a specific location has been configured */
		bool has_specific_location() const { return mLocation.has_value(); }
		/** The location or 0 if no specific location has been configured */
		uint32_t location() const { return mLocation.value_or(0u); }
		/** True if this attachment shall be, finally, presented on screen. */
		auto is_presentable() const { return has_flag(mImageUsage, cgb::image_usage::presentable) || has_flag(mImageUsage, cgb::image_usage::shared_presentable); }
		/** True if this attachment represents a depth/stencil attachment. */
		auto is_depth_stencil_attachment() const { return has_flag(mImageUsage, cgb::image_usage::depth_stencil_attachment); }
		/** True if this attachment represents not a depth/stencil attachment, but instead, a color attachment */
		auto is_color_attachment() const { return !is_depth_stencil_attachment(); }
		/** True fi the sample count is greater than 1 */
		bool is_multisampled() const { return mSampleCount > 1; }
		/** The sample count for this attachment. */
		auto sample_count() const { return mSampleCount; }
		/** True if a multisample resolve pass shall be set up. */
		auto is_to_be_resolved() const { return mDoResolve; }
		/** True if this attachment is intended to be used as input attachment to a shader. */
		auto is_shader_input_attachment() const { return has_flag(mImageUsage, cgb::image_usage::input_attachment); }

		/** Returns the stencil load operation */
		auto get_stencil_load_op() const { return mStencilLoadOperation.value_or(mLoadOperation); }
		/** Returns the stencil store operation */
		auto get_stencil_store_op() const { return mStencilStoreOperation.value_or(mStoreOperation); }

		std::optional<uint32_t> mLocation;
		image_format mFormat;
		image_usage mImageUsage;
		cfg::attachment_load_operation mLoadOperation;
		cfg::attachment_store_operation mStoreOperation;
		std::optional<cfg::attachment_load_operation> mStencilLoadOperation;
		std::optional<cfg::attachment_store_operation> mStencilStoreOperation;
		int mSampleCount;
		bool mDoResolve;
	};
}
