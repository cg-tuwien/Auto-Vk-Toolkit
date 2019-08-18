#pragma once

namespace cgb
{
	/** Configuration data for a binding, containing a set-index, binding data, 
	*	and the shader stages where the bound resource might be used.
	*/
	struct binding_data
	{
		uint32_t mSetId;
		vk::DescriptorSetLayoutBinding mLayoutBinding;
		shader_type mShaderStages;
		std::variant<
			std::monostate,
			generic_buffer_t*,
			uniform_buffer_t*,
			uniform_texel_buffer_t*,
			storage_buffer_t*,
			storage_texel_buffer_t*,
			vertex_buffer_t*,
			index_buffer_t*,
			instance_buffer_t*,
			acceleration_structure*,
			image_view_t*,
			sampler_t*,
			image_sampler_t*,
			std::vector<generic_buffer_t*>,
			std::vector<uniform_buffer_t*>,
			std::vector<uniform_texel_buffer_t*>,
			std::vector<storage_buffer_t*>,
			std::vector<storage_texel_buffer_t*>,
			std::vector<vertex_buffer_t*>,
			std::vector<index_buffer_t*>,
			std::vector<instance_buffer_t*>,
			std::vector<acceleration_structure*>,
			std::vector<image_view_t*>,
			std::vector<sampler_t*>,
			std::vector<image_sampler_t*>
		> mResourcePtr;

		const vk::DescriptorImageInfo* descriptor_image_info() const
		{
			if (std::holds_alternative<generic_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<uniform_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<uniform_texel_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<storage_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<storage_texel_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<vertex_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<index_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<instance_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<acceleration_structure*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<image_view_t*>(mResourcePtr)) { return &std::get<image_view_t*>(mResourcePtr)->descriptor_info(); }
			if (std::holds_alternative<sampler_t*>(mResourcePtr)) { return &std::get<sampler_t*>(mResourcePtr)->descriptor_info(); }
			if (std::holds_alternative<image_sampler_t*>(mResourcePtr)) { return &std::get<image_sampler_t*>(mResourcePtr)->descriptor_info(); }
			// TODO: Handle array types!
			throw std::runtime_error("Some holds_alternative calls are not implemented.");
		}

		const vk::DescriptorBufferInfo* descriptor_buffer_info() const
		{
			if (std::holds_alternative<generic_buffer_t*>(mResourcePtr)) { return &std::get<generic_buffer_t*>(mResourcePtr)->descriptor_info(); }
			if (std::holds_alternative<uniform_buffer_t*>(mResourcePtr)) { return &std::get<uniform_buffer_t*>(mResourcePtr)->descriptor_info(); }
			if (std::holds_alternative<uniform_texel_buffer_t*>(mResourcePtr)) { return &std::get<uniform_texel_buffer_t*>(mResourcePtr)->descriptor_info(); }
			if (std::holds_alternative<storage_buffer_t*>(mResourcePtr)) { return &std::get<storage_buffer_t*>(mResourcePtr)->descriptor_info(); }
			if (std::holds_alternative<storage_texel_buffer_t*>(mResourcePtr)) { return &std::get<storage_texel_buffer_t*>(mResourcePtr)->descriptor_info(); }
			if (std::holds_alternative<vertex_buffer_t*>(mResourcePtr)) { return &std::get<vertex_buffer_t*>(mResourcePtr)->descriptor_info(); }
			if (std::holds_alternative<index_buffer_t*>(mResourcePtr)) { return &std::get<index_buffer_t*>(mResourcePtr)->descriptor_info(); }
			if (std::holds_alternative<instance_buffer_t*>(mResourcePtr)) { return &std::get<index_buffer_t*>(mResourcePtr)->descriptor_info(); }
			if (std::holds_alternative<acceleration_structure*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<image_view_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<sampler_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<image_sampler_t*>(mResourcePtr)) { return nullptr; }
			// TODO: Handle array types!
			throw std::runtime_error("Some holds_alternative calls are not implemented.");
		}

		const void* next_pointer() const
		{
			if (std::holds_alternative<generic_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<uniform_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<uniform_texel_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<storage_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<storage_texel_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<vertex_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<index_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<instance_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<acceleration_structure*>(mResourcePtr)) { return &std::get<acceleration_structure*>(mResourcePtr)->descriptor_info(); }
			if (std::holds_alternative<image_view_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<sampler_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<image_sampler_t*>(mResourcePtr)) { return nullptr; }
			// TODO: Handle array types!
			throw std::runtime_error("Some holds_alternative calls are not implemented.");
		}

		const vk::BufferView* texel_buffer_info() const
		{
			if (std::holds_alternative<generic_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<uniform_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<uniform_texel_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<storage_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<storage_texel_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<vertex_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<index_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<instance_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<acceleration_structure*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<image_view_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<sampler_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<image_sampler_t*>(mResourcePtr)) { return nullptr; }
			// TODO: Handle array types!
			throw std::runtime_error("Some holds_alternative calls are not implemented.");
		}
	};

	/** Compares two `binding_data` instances for equality, but only in
	*	in terms of their set-ids and binding-ids. 
	*	It does not consider equality or inequality of other members 
	*  (like of the `mLayoutBinding` or the `mShaderStages` members - they are simply ignored)
	*/
	inline bool operator ==(const binding_data& first, const binding_data& second)
	{
		return first.mSetId == second.mSetId
			&& first.mLayoutBinding.binding == second.mLayoutBinding.binding;
	}

	/** Compares two `binding_data` instances for inequality, but only in
	*	in terms of their set-ids and binding-ids. 
	*	It does not consider equality or inequality of other members 
	*  (like of the `mLayoutBinding` or the `mShaderStages` members - they are simply ignored)
	*/
	inline bool operator !=(const binding_data& first, const binding_data& second)
	{
		return !(first == second);
	}

	/** Returns true if the first binding is less than the second binding
	*	in terms of their set-ids and binding-ids. 
	*	It does not consider other members (like of the `mLayoutBinding` 
	*	or the `mShaderStages` members - they are simply ignored)
	*/
	inline bool operator <(const binding_data& first, const binding_data& second)
	{
		return	first.mSetId < second.mSetId
			|| (first.mSetId == second.mSetId && first.mLayoutBinding.binding < second.mLayoutBinding.binding);
	}

	/** Returns true if the first binding is less than or equal to the second binding
	*	in terms of their set-ids and binding-ids. 
	*	It does not consider other members (like of the `mLayoutBinding` 
	*	or the `mShaderStages` members - they are simply ignored)
	*/
	inline bool operator <=(const binding_data& first, const binding_data& second)
	{
		return first < second || first == second;
	}

	/** Returns true if the first binding is greater than the second binding
	*	in terms of their set-ids and binding-ids. 
	*	It does not consider other members (like of the `mLayoutBinding` 
	*	or the `mShaderStages` members - they are simply ignored)
	*/
	inline bool operator >(const binding_data& first, const binding_data& second)
	{
		return !(first <= second);
	}

	/** Returns true if the first binding is greater than or equal to the second binding
	*	in terms of their set-ids and binding-ids. 
	*	It does not consider other members (like of the `mLayoutBinding` 
	*	or the `mShaderStages` members - they are simply ignored)
	*/
	inline bool operator >=(const binding_data& first, const binding_data& second)
	{
		return !(first < second);
	}
}
