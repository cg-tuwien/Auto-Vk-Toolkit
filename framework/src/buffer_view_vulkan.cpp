#include <cg_base.hpp>

namespace cgb
{
	bool buffer_view_t::has_cgb_uniform_texel_buffer() const
	{
		return std::holds_alternative<cgb::uniform_texel_buffer>(mBuffer);
	}
	const uniform_texel_buffer_t& buffer_view_t::get_uniform_texel_buffer() const
	{
		if (!has_cgb_uniform_texel_buffer()) {
			throw std::logic_error("This `cgb::buffer_view_t` is not associated to a `cgb::uniform_texel_buffer`");
		}
		return std::get<cgb::uniform_texel_buffer>(mBuffer);
	}
	uniform_texel_buffer_t& buffer_view_t::get_uniform_texel_buffer()
	{
		if (!has_cgb_uniform_texel_buffer()) {
			throw std::logic_error("This `cgb::buffer_view_t` is not associated to a `cgb::uniform_texel_buffer`");
		}
		return std::get<cgb::uniform_texel_buffer>(mBuffer);
	}

	bool buffer_view_t::has_cgb_storage_texel_buffer() const
	{
		return std::holds_alternative<cgb::storage_texel_buffer>(mBuffer);
	}
	const storage_texel_buffer_t& buffer_view_t::get_storage_texel_buffer() const
	{
		if (!has_cgb_storage_texel_buffer()) {
			throw std::logic_error("This `cgb::buffer_view_t` is not associated to a `cgb::storage_texel_buffer`");
		}
		return std::get<cgb::storage_texel_buffer>(mBuffer);
	}
	storage_texel_buffer_t& buffer_view_t::get_storage_texel_buffer()
	{
		if (!has_cgb_storage_texel_buffer()) {
			throw std::logic_error("This `cgb::buffer_view_t` is not associated to a `cgb::storage_texel_buffer`");
		}
		return std::get<cgb::storage_texel_buffer>(mBuffer);
	}

	const vk::Buffer& buffer_view_t::buffer_handle() const
	{
		if (std::holds_alternative<cgb::uniform_texel_buffer>(mBuffer)) {
			return std::get<cgb::uniform_texel_buffer>(mBuffer)->buffer_handle();
		}
		if (std::holds_alternative<cgb::storage_texel_buffer>(mBuffer)) {
			return std::get<cgb::storage_texel_buffer>(mBuffer)->buffer_handle();
		}
		return std::get<vk::Buffer>(std::get<std::tuple<vk::Buffer, vk::BufferCreateInfo>>(mBuffer));	
	}

	const vk::BufferCreateInfo& buffer_view_t::buffer_config() const
	{
		if (std::holds_alternative<cgb::uniform_texel_buffer>(mBuffer)) {
			return std::get<cgb::uniform_texel_buffer>(mBuffer)->config();
		}
		if (std::holds_alternative<cgb::storage_texel_buffer>(mBuffer)) {
			return std::get<cgb::storage_texel_buffer>(mBuffer)->config();
		}
		return std::get<vk::BufferCreateInfo>(std::get<std::tuple<vk::Buffer, vk::BufferCreateInfo>>(mBuffer));
	}

	vk::DescriptorType buffer_view_t::descriptor_type() const
	{
		if (std::holds_alternative<cgb::uniform_texel_buffer>(mBuffer)) {
			return std::get<cgb::uniform_texel_buffer>(mBuffer)->descriptor_type();
		}
		if (std::holds_alternative<cgb::storage_texel_buffer>(mBuffer)) {
			return std::get<cgb::storage_texel_buffer>(mBuffer)->descriptor_type();
		}
		throw std::runtime_error("Which descriptor type?");
	}
	
	owning_resource<buffer_view_t> buffer_view_t::create(cgb::uniform_texel_buffer _BufferToOwn, std::optional<buffer_member_format> _ViewFormat, context_specific_function<void(buffer_view_t&)> _AlterConfigBeforeCreation)
	{
		buffer_view_t result;
		buffer_member_format format;
		if (_ViewFormat.has_value()) {
			format = _ViewFormat.value();
		}
		else {
			if (_BufferToOwn->meta_data().member_descriptions().size() == 0) {
				throw std::runtime_error("No _ViewFormat passed and cgb::uniform_texel_buffer contains no member descriptions");
			}
			if (_BufferToOwn->meta_data().member_descriptions().size() > 1) {
				LOG_WARNING("No _ViewFormat passed and there is more than one member description in cgb::uniform_texel_buffer. The view will likely be corrupted.");
			}
			format = _BufferToOwn->meta_data().member_descriptions().front().mFormat;
		}
		// Transfer ownership:
		result.mBuffer = std::move(_BufferToOwn);
		result.finish_configuration(format, std::move(_AlterConfigBeforeCreation));
		return result;
	}
	
	owning_resource<buffer_view_t> buffer_view_t::create(cgb::storage_texel_buffer _BufferToOwn, std::optional<buffer_member_format> _ViewFormat, context_specific_function<void(buffer_view_t&)> _AlterConfigBeforeCreation)
	{
		buffer_view_t result;
		buffer_member_format format;
		if (_ViewFormat.has_value()) {
			format = _ViewFormat.value();
		}
		else {
			if (_BufferToOwn->meta_data().member_descriptions().size() == 0) {
				throw std::runtime_error("No _ViewFormat passed and cgb::storage_texel_buffer contains no member descriptions");
			}
			if (_BufferToOwn->meta_data().member_descriptions().size() > 1) {
				LOG_WARNING("No _ViewFormat passed and there is more than one member description in cgb::storage_texel_buffer. The view will likely be corrupted.");
			}
			format = _BufferToOwn->meta_data().member_descriptions().front().mFormat;
		}
		// Transfer ownership:
		result.mBuffer = std::move(_BufferToOwn);
		result.finish_configuration(format, std::move(_AlterConfigBeforeCreation));
		return result;
	}
	
	owning_resource<buffer_view_t> buffer_view_t::create(vk::Buffer _BufferToReference, vk::BufferCreateInfo _BufferInfo, buffer_member_format _ViewFormat, context_specific_function<void(buffer_view_t&)> _AlterConfigBeforeCreation)
	{
		buffer_view_t result;
		// Store handles:
		result.mBuffer = std::make_tuple(_BufferToReference, _BufferInfo);
		result.finish_configuration(_ViewFormat, std::move(_AlterConfigBeforeCreation));
		return result;
	}

	void buffer_view_t::finish_configuration(buffer_member_format _ViewFormat, context_specific_function<void(buffer_view_t&)> _AlterConfigBeforeCreation)
	{
		mInfo = vk::BufferViewCreateInfo{}
			.setBuffer(buffer_handle())
			.setFormat(_ViewFormat.mFormat)
			.setOffset(0) // TODO: Support offsets
			.setRange(VK_WHOLE_SIZE); // TODO: Support ranges

		// Maybe alter the config?!
		if (_AlterConfigBeforeCreation.mFunction) {
			_AlterConfigBeforeCreation.mFunction(*this);
		}

		mBufferView = context().logical_device().createBufferViewUnique(mInfo);

		// TODO: Descriptors?!

		mTracker.setTrackee(*this);
	}
}
