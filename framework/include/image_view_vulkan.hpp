#pragma once

namespace cgb
{
	/** Class representing an image view. */
	class image_view_t
	{
	public:
		image_view_t() = default;
		image_view_t(const image_view_t&) = delete;
		image_view_t(image_view_t&&) = default;
		image_view_t& operator=(const image_view_t&) = delete;
		image_view_t& operator=(image_view_t&&) = default;
		~image_view_t() = default;

		/** Get the config which is used to created this image view with the API. */
		const auto& config() const { return mInfo; }
		/** Get the config which is used to created this image view with the API. */
		auto& config() { return mInfo; }
		/** Gets the image handle which this view has been created for. */
		const vk::Image& image_handle() const;
		/** Gets the images's config */
		const vk::ImageCreateInfo& image_config() const;
		/** Gets the image view's vulkan handle */
		const auto& view_handle() const { return mImageView.get(); }

		const auto& descriptor_info() const		{ return mDescriptorInfo; }
		const auto& descriptor_type() const		{ return mDescriptorType; }

		/** Creates a new image view upon a given image
		*	@param	_ImageToOwn					The image which to create an image view for
		*	@param	_ViewFormat					The format of the image view. If none is specified, it will be set to the same format as the image.
		*	@param	_AlterConfigBeforeCreation	A context-specific function which allows to modify the `vk::ImageViewCreateInfo` just before the image view will be created. Use `.config()` to access the configuration structure!
		*	@return	Returns a newly created image.
		*/
		static owning_resource<image_view_t> create(cgb::image _ImageToOwn, std::optional<image_format> _ViewFormat = std::nullopt, context_specific_function<void(image_view_t&)> _AlterConfigBeforeCreation = {});

		static owning_resource<image_view_t> create(vk::Image _ImageToReference, vk::ImageCreateInfo _ImageInfo, std::optional<image_format> _ViewFormat = std::nullopt, context_specific_function<void(image_view_t&)> _AlterConfigBeforeCreation = {});

	private:
		void finish_configuration(image_format _ViewFormat, context_specific_function<void(image_view_t&)> _AlterConfigBeforeCreation);

		// Owning XOR non-owning handle to an image. (Make sure it gets destructed after the image view if it is owning)
		std::variant<cgb::image, std::tuple<vk::Image, vk::ImageCreateInfo>> mImage;
		// Config which is passed to the create call and contains all the parameters for image view creation.
		vk::ImageViewCreateInfo mInfo;
		// The image view's handle. This member will contain a valid handle only after successful image view creation.
		vk::UniqueImageView mImageView;
		vk::DescriptorImageInfo mDescriptorInfo;
		vk::DescriptorType mDescriptorType;
		context_tracker<image_view_t> mTracker;
	};

	/** Typedef representing any kind of OWNING image view representations. */
	using image_view = owning_resource<image_view_t>;
}
