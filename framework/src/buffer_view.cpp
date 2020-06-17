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
			throw cgb::logic_error("This `cgb::buffer_view_t` is not associated to a `cgb::uniform_texel_buffer`");
		}
		return std::get<cgb::uniform_texel_buffer>(mBuffer);
	}
	uniform_texel_buffer_t& buffer_view_t::get_uniform_texel_buffer()
	{
		if (!has_cgb_uniform_texel_buffer()) {
			throw cgb::logic_error("This `cgb::buffer_view_t` is not associated to a `cgb::uniform_texel_buffer`");
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
			throw cgb::logic_error("This `cgb::buffer_view_t` is not associated to a `cgb::storage_texel_buffer`");
		}
		return std::get<cgb::storage_texel_buffer>(mBuffer);
	}
	storage_texel_buffer_t& buffer_view_t::get_storage_texel_buffer()
	{
		if (!has_cgb_storage_texel_buffer()) {
			throw cgb::logic_error("This `cgb::buffer_view_t` is not associated to a `cgb::storage_texel_buffer`");
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
		throw cgb::runtime_error("Which descriptor type?");
	}
	
	owning_resource<buffer_view_t> buffer_view_t::create(cgb::uniform_texel_buffer _BufferToOwn, std::optional<buffer_member_format> _ViewFormat, context_specific_function<void(buffer_view_t&)> _AlterConfigBeforeCreation)
	{
		
	}
	
	owning_resource<buffer_view_t> buffer_view_t::create(cgb::storage_texel_buffer _BufferToOwn, std::optional<buffer_member_format> _ViewFormat, context_specific_function<void(buffer_view_t&)> _AlterConfigBeforeCreation)
	{
		
	}
	
	owning_resource<buffer_view_t> buffer_view_t::create(vk::Buffer _BufferToReference, vk::BufferCreateInfo _BufferInfo, buffer_member_format _ViewFormat, context_specific_function<void(buffer_view_t&)> _AlterConfigBeforeCreation)
	{
		
	}

}
