#pragma once

namespace cgb
{
	class image_sampler_t
	{
	public:
		image_sampler_t() = default;
		image_sampler_t(image_sampler_t&&) noexcept = default;
		image_sampler_t(const image_sampler_t&) = delete;
		image_sampler_t& operator=(image_sampler_t&&) noexcept = default;
		image_sampler_t& operator=(const image_sampler_t&) = delete;
		~image_sampler_t() = default;

		const auto& get_image_view() const		{ return mImageView; }
		auto& get_image_view()					{ return mImageView; }
		const auto& get_sampler() const			{ return mSampler; }
		const auto& view_handle() const			{ return mImageView->handle(); }
		const auto& image_handle() const		{ return mImageView->get_image().handle(); }
		const auto& sampler_handle() const		{ return mSampler->handle(); }
		const auto& descriptor_info() const		{ return mDescriptorInfo; }
		const auto& descriptor_type() const		{ return mDescriptorType; }
		/** Gets the width of the image */
		uint32_t width() const { return mImageView->get_image().width(); }
		/** Gets the height of the image */
		uint32_t height() const { return mImageView->get_image().height(); }
		/** Gets the depth of the image */
		uint32_t depth() const { return mImageView->get_image().depth(); }
		/** Gets the format of the image */
		image_format format() const { return mImageView->get_image().format(); }

		static owning_resource<image_sampler_t> create(image_view pImageView, sampler pSampler);

	private:
		image_view mImageView;
		sampler mSampler;
		vk::DescriptorImageInfo mDescriptorInfo;
		vk::DescriptorType mDescriptorType;
	};

	/** Typedef representing any kind of OWNING image-sampler representations. */
	using image_sampler = owning_resource<image_sampler_t>;

	/** Compares two `image_sampler_t`s for equality.
	 *	They are considered equal if all their handles (image, image-view, sampler) are the same.
	 *	The config structs or the descriptor data is not evaluated for equality comparison.
	 */
	static bool operator==(const image_sampler_t& left, const image_sampler_t& right)
	{
		return left.view_handle() == right.view_handle()
			&& left.image_handle() == right.image_handle()
			&& left.sampler_handle() == right.sampler_handle();
	}

	/** Returns `true` if the two `image_sampler_t`s are not equal. */
	static bool operator!=(const image_sampler_t& left, const image_sampler_t& right)
	{
		return !(left == right);
	}
}

namespace std // Inject hash for `cgb::image_sampler_t` into std::
{
	template<> struct hash<cgb::image_sampler_t>
	{
		std::size_t operator()(cgb::image_sampler_t const& o) const noexcept
		{
			std::size_t h = 0;
			cgb::hash_combine(h,
				static_cast<VkImageView>(o.view_handle()),
				static_cast<VkImage>(o.image_handle()),
				static_cast<VkSampler>(o.sampler_handle())
			);
			return h;
		}
	};
}
