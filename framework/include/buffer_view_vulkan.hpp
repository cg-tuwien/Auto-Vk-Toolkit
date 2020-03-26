#pragma once

namespace cgb
{
	/** Class representing a buffer view, which "wraps" a uniform texel buffer or a storage texel buffer */
	class buffer_view_t
	{
	public:
		buffer_view_t() noexcept = default;
		buffer_view_t(const buffer_view_t&) = delete;
		buffer_view_t(buffer_view_t&&) noexcept = default;
		buffer_view_t& operator=(const buffer_view_t&) = delete;
		buffer_view_t& operator=(buffer_view_t&&) noexcept = default;
		~buffer_view_t() = default;

		/** Get the config which is used to created this image view with the API. */
		const auto& config() const { return mInfo; }
		/** Get the config which is used to created this image view with the API. */
		auto& config() { return mInfo; }
		
		/** Returns true if it holds a `cgb::uniform_texel_buffer` */
		bool has_cgb_uniform_texel_buffer() const;
		/** Gets the associated image or throws if no `cgb::uniform_texel_buffer` is associated. */
		const uniform_texel_buffer_t& get_uniform_texel_buffer() const;
		/** Gets the associated image or throws if no `cgb::uniform_texel_buffer` is associated. */
		uniform_texel_buffer_t& get_uniform_texel_buffer();
		
		/** Returns true if it holds a `cgb::storage_texel_buffer` */
		bool has_cgb_storage_texel_buffer() const;
		/** Gets the associated image or throws if no `cgb::storage_texel_buffer` is associated. */
		const storage_texel_buffer_t& get_storage_texel_buffer() const;
		/** Gets the associated image or throws if no `cgb::storage_texel_buffer` is associated. */
		storage_texel_buffer_t& get_storage_texel_buffer();
		
		/** Gets the buffer handle which this view has been created for. */
		const vk::Buffer& buffer_handle() const;
		/** Gets the buffer's config */
		const vk::BufferCreateInfo& buffer_config() const;
		
		/** Gets the buffer view's vulkan handle */
		const auto& view_handle() const { return mBufferView.get(); }

		/** Gets the descriptor type from the wrapped buffer */
		vk::DescriptorType descriptor_type() const;

		static owning_resource<buffer_view_t> create(cgb::uniform_texel_buffer _BufferToOwn, std::optional<buffer_member_format> _ViewFormat = {}, context_specific_function<void(buffer_view_t&)> _AlterConfigBeforeCreation = {});
		static owning_resource<buffer_view_t> create(cgb::storage_texel_buffer _BufferToOwn, std::optional<buffer_member_format> _ViewFormat = {}, context_specific_function<void(buffer_view_t&)> _AlterConfigBeforeCreation = {});
		static owning_resource<buffer_view_t> create(vk::Buffer _BufferToReference, vk::BufferCreateInfo _BufferInfo, buffer_member_format _ViewFormat, context_specific_function<void(buffer_view_t&)> _AlterConfigBeforeCreation = {});

	private:
		void finish_configuration(buffer_member_format _ViewFormat, context_specific_function<void(buffer_view_t&)> _AlterConfigBeforeCreation);

		// Owning XOR non-owning handle to an buffer.
		std::variant<cgb::uniform_texel_buffer, cgb::storage_texel_buffer, std::tuple<vk::Buffer, vk::BufferCreateInfo>> mBuffer;
		// Config which is passed to the create call and contains all the parameters for buffer view creation.
		vk::BufferViewCreateInfo mInfo;
		// The image view's handle. This member will contain a valid handle only after successful image view creation.
		vk::UniqueBufferView mBufferView;
		
		context_tracker<buffer_view_t> mTracker;
	};

	/** Typedef representing any kind of OWNING image view representations. */
	using buffer_view = owning_resource<buffer_view_t>;

	/** Compares two `buffer_view_t`s for equality.
	 *	They are considered equal if all their handles (buffer, buffer-view) are the same.
	 *	The config structs or the descriptor data is not evaluated for equality comparison.
	 */
	static bool operator==(const buffer_view_t& left, const buffer_view_t& right)
	{
		return left.view_handle() == right.view_handle()
			&& left.buffer_handle() == right.buffer_handle();
	}

	/** Returns `true` if the two `buffer_view_t`s are not equal. */
	static bool operator!=(const buffer_view_t& left, const buffer_view_t& right)
	{
		return !(left == right);
	}
}

namespace std // Inject hash for `cgb::image_sampler_t` into std::
{
	template<> struct hash<cgb::buffer_view_t>
	{
		std::size_t operator()(cgb::buffer_view_t const& o) const noexcept
		{
			std::size_t h = 0;
			cgb::hash_combine(h,
				static_cast<VkBufferView>(o.view_handle()),
				static_cast<VkBuffer>(o.buffer_handle())
			);
			return h;
		}
	};
}

