#pragma once

namespace cgb
{
	class image_view_as_input_attachment
	{
		friend class image_view_t;
	public:
		const auto& descriptor_info() const		{ return mDescriptorInfo; }
		
	private:
		vk::DescriptorImageInfo mDescriptorInfo;
	};
	
	/** Class representing an image view. */
	class image_view_t
	{
		struct helper_t
		{
			helper_t(image_t aImage) : mImage{std::move(aImage)} {}
			helper_t(helper_t&&) noexcept = default;
			helper_t(const helper_t&) noexcept = default;
			helper_t& operator=(helper_t&&) noexcept = default;
			helper_t& operator=(const helper_t&) noexcept = delete;
			~helper_t() = default;
			
			image_t mImage;
		};
		
	public:
		image_view_t() = default;
		image_view_t(image_view_t&&) noexcept = default;
		image_view_t(const image_view_t&) = delete;
		image_view_t& operator=(image_view_t&&) noexcept = default;
		image_view_t& operator=(const image_view_t&) = delete;
		~image_view_t() = default;

		/** Get the config which is used to created this image view with the API. */
		const auto& config() const { return mInfo; }
		/** Get the config which is used to created this image view with the API. */
		auto& config() { return mInfo; }

		/** Gets the associated image or throws if no `cgb::image` is associated. */
		const image_t& get_image() const
		{
			assert(!std::holds_alternative<std::monostate>(mImage));
			return std::holds_alternative<helper_t>(mImage) ? std::get<helper_t>(mImage).mImage : static_cast<const image_t&>(std::get<image>(mImage));
		}

		/** Gets the associated image or throws if no `cgb::image` is associated. */
		image_t& get_image() 
		{
			assert(!std::holds_alternative<std::monostate>(mImage));
			return std::holds_alternative<helper_t>(mImage) ? std::get<helper_t>(mImage).mImage : static_cast<image_t&>(std::get<image>(mImage));
		}
		
		/** Gets the image view's vulkan handle */
		const auto& handle() const { return mImageView.get(); }

		const auto& descriptor_info() const		{ return mDescriptorInfo; }

		image_view_as_input_attachment as_input_attachment() const;

		/** Creates a new image view upon a given image
		*	@param	aImageToOwn					The image which to create an image view for
		*	@param	aViewFormat					The format of the image view. If none is specified, it will be set to the same format as the image.
		*	@param	aAlterConfigBeforeCreation	A context-specific function which allows to modify the `vk::ImageViewCreateInfo` just before the image view will be created. Use `.config()` to access the configuration structure!
		*	@return	Returns a newly created image.
		*/
		static owning_resource<image_view_t> create(cgb::image aImageToOwn, std::optional<image_format> aViewFormat = std::nullopt, context_specific_function<void(image_view_t&)> aAlterConfigBeforeCreation = {});

		static owning_resource<image_view_t> create(cgb::image_t aImageToWrap, std::optional<image_format> aViewFormat = std::nullopt);

	private:
		void finish_configuration(image_format _ViewFormat, context_specific_function<void(image_view_t&)> _AlterConfigBeforeCreation);

		// The "wrapped" image:
		std::variant<std::monostate, helper_t, cgb::image> mImage;
		// Config which is passed to the create call and contains all the parameters for image view creation.
		vk::ImageViewCreateInfo mInfo;
		// The image view's handle. This member will contain a valid handle only after successful image view creation.
		vk::UniqueImageView mImageView;
		vk::DescriptorImageInfo mDescriptorInfo;
		context_tracker<image_view_t> mTracker;
	};

	/** Typedef representing any kind of OWNING image view representations. */
	using image_view = owning_resource<image_view_t>;

	/** Compares two `image_view_t`s for equality.
	 *	They are considered equal if all their handles (image, image-view) are the same.
	 *	The config structs or the descriptor data is not evaluated for equality comparison.
	 */
	static bool operator==(const image_view_t& left, const image_view_t& right)
	{
		return left.handle() == right.handle()
			&& left.get_image().handle() == right.get_image().handle();
	}

	/** Returns `true` if the two `image_view_t`s are not equal. */
	static bool operator!=(const image_view_t& left, const image_view_t& right)
	{
		return !(left == right);
	}
}

namespace std // Inject hash for `cgb::image_sampler_t` into std::
{
	template<> struct hash<cgb::image_view_t>
	{
		std::size_t operator()(cgb::image_view_t const& o) const noexcept
		{
			std::size_t h = 0;
			cgb::hash_combine(h,
				static_cast<VkImageView>(o.handle()),
				static_cast<VkImage>(o.get_image().handle())
			);
			return h;
		}
	};
}
