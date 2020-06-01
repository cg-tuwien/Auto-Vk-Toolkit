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
		std::variant<
			std::monostate,
			const generic_buffer_t*,
			const uniform_buffer_t*,
			const uniform_texel_buffer_t*,
			const storage_buffer_t*,
			const storage_texel_buffer_t*,
			const vertex_buffer_t*,
			const index_buffer_t*,
			const instance_buffer_t*,
			const top_level_acceleration_structure_t*,
			const image_view_t*,
			const image_view_as_input_attachment*,
			const image_view_as_storage_image*,
			const buffer_view_t*,
			const sampler_t*,
			const image_sampler_t*,
			std::vector<const generic_buffer_t*>,
			std::vector<const uniform_buffer_t*>,
			std::vector<const uniform_texel_buffer_t*>,
			std::vector<const storage_buffer_t*>,
			std::vector<const storage_texel_buffer_t*>,
			std::vector<const vertex_buffer_t*>,
			std::vector<const index_buffer_t*>,
			std::vector<const instance_buffer_t*>,
			std::vector<const top_level_acceleration_structure_t*>,
			std::vector<const image_view_t*>,
			std::vector<const image_view_as_input_attachment*>,
			std::vector<const image_view_as_storage_image*>,
			std::vector<const buffer_view_t*>,
			std::vector<const sampler_t*>,
			std::vector<const image_sampler_t*>
		> mResourcePtr;


		template <typename T>
		std::vector<vk::DescriptorImageInfo> gather_image_infos(const std::vector<T*>& vec) const
		{
			std::vector<vk::DescriptorImageInfo> dataForImageInfos;
			for (auto& v : vec) {
				dataForImageInfos.push_back(v->descriptor_info());
			}
			return dataForImageInfos;
		}

		template <typename T>
		std::vector<vk::DescriptorBufferInfo> gather_buffer_infos(const std::vector<T*>& vec) const
		{
			std::vector<vk::DescriptorBufferInfo> dataForBufferInfos;
			for (auto& v : vec) {
				dataForBufferInfos.push_back(v->descriptor_info());
			}
			return dataForBufferInfos;
		}

		template <typename T>
		std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> gather_acceleration_structure_infos(const std::vector<T*>& vec) const
		{
			std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> dataForAccStructures;
			for (auto& v : vec) {
				dataForAccStructures.push_back(v->descriptor_info());
			}
			return dataForAccStructures;
		}

		template <typename T>
		std::vector<vk::BufferView> gather_buffer_views(const std::vector<T*>& vec) const
		{
			std::vector<vk::BufferView> dataForBufferViews;
			for (auto& v : vec) {
				dataForBufferViews.push_back(v->view_handle());
			}
			return dataForBufferViews;
		}

		uint32_t descriptor_count() const
		{
			if (std::holds_alternative<std::vector<const generic_buffer_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const generic_buffer_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<const uniform_buffer_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const uniform_buffer_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<const uniform_texel_buffer_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const uniform_texel_buffer_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<const storage_buffer_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const storage_buffer_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<const storage_texel_buffer_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const storage_texel_buffer_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<const vertex_buffer_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const vertex_buffer_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<const index_buffer_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const index_buffer_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<const instance_buffer_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const instance_buffer_t*>>(mResourcePtr).size()); }
			// vvv NOPE vvv There can only be ONE pNext (at least I think so)
			//if (std::holds_alternative<std::vector<const top_level_acceleration_structure_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const top_level_acceleration_structure_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<const image_view_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const image_view_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<const image_view_as_input_attachment*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const image_view_as_input_attachment*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<const image_view_as_storage_image*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const image_view_as_storage_image*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<const sampler_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const sampler_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<const image_sampler_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const image_sampler_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<const buffer_view_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<const buffer_view_t*>>(mResourcePtr).size()); }
			return 1u;
		}

		const vk::DescriptorImageInfo* descriptor_image_info(descriptor_set& aDescriptorSet) const
		{
			if (std::holds_alternative<const generic_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const uniform_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const uniform_texel_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const storage_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const storage_texel_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const vertex_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const index_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const instance_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const top_level_acceleration_structure_t*>(mResourcePtr)) { return nullptr; }
			
			if (std::holds_alternative<const image_view_t*>(mResourcePtr)) {
				return aDescriptorSet.store_image_info(mLayoutBinding.binding, std::get<const image_view_t*>(mResourcePtr)->descriptor_info());
			}
			if (std::holds_alternative<const image_view_as_input_attachment*>(mResourcePtr)) {
				return aDescriptorSet.store_image_info(mLayoutBinding.binding, std::get<const image_view_as_input_attachment*>(mResourcePtr)->descriptor_info());
			}
			if (std::holds_alternative<const image_view_as_storage_image*>(mResourcePtr)) { 
				return aDescriptorSet.store_image_info(mLayoutBinding.binding, std::get<const image_view_as_storage_image*>(mResourcePtr)->descriptor_info());
			}
			if (std::holds_alternative<const sampler_t*>(mResourcePtr)) {  
				return aDescriptorSet.store_image_info(mLayoutBinding.binding, std::get<const sampler_t*>(mResourcePtr)->descriptor_info());
			}
			if (std::holds_alternative<const image_sampler_t*>(mResourcePtr)) { 
				return aDescriptorSet.store_image_info(mLayoutBinding.binding, std::get<const image_sampler_t*>(mResourcePtr)->descriptor_info());
			}

			if (std::holds_alternative<const buffer_view_t*>(mResourcePtr)) { return nullptr; }

			if (std::holds_alternative<std::vector<const generic_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const uniform_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const uniform_texel_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const storage_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const storage_texel_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const vertex_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const index_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const instance_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const top_level_acceleration_structure_t*>>(mResourcePtr)) { return nullptr; }

			if (std::holds_alternative<std::vector<const image_view_t*>>(mResourcePtr)) { 
				return aDescriptorSet.store_image_infos(mLayoutBinding.binding, gather_image_infos(std::get<std::vector<const image_view_t*>>(mResourcePtr)));
			}
			if (std::holds_alternative<std::vector<const image_view_as_input_attachment*>>(mResourcePtr)) { 
				return aDescriptorSet.store_image_infos(mLayoutBinding.binding, gather_image_infos(std::get<std::vector<const image_view_as_input_attachment*>>(mResourcePtr)));
			}
			if (std::holds_alternative<std::vector<const image_view_as_storage_image*>>(mResourcePtr)) { 
				return aDescriptorSet.store_image_infos(mLayoutBinding.binding, gather_image_infos(std::get<std::vector<const image_view_as_storage_image*>>(mResourcePtr)));
			}
			if (std::holds_alternative<std::vector<const sampler_t*>>(mResourcePtr)) { 
				return aDescriptorSet.store_image_infos(mLayoutBinding.binding, gather_image_infos(std::get<std::vector<const sampler_t*>>(mResourcePtr)));
			}
			if (std::holds_alternative<std::vector<const image_sampler_t*>>(mResourcePtr)) { 
				return aDescriptorSet.store_image_infos(mLayoutBinding.binding, gather_image_infos(std::get<std::vector<const image_sampler_t*>>(mResourcePtr)));
			}
			
			if (std::holds_alternative<std::vector<const buffer_view_t*>>(mResourcePtr)) { return nullptr; }

			throw cgb::runtime_error("Some holds_alternative calls are not implemented.");
		}

		const vk::DescriptorBufferInfo* descriptor_buffer_info(descriptor_set& aDescriptorSet) const
		{
			if (std::holds_alternative<const generic_buffer_t*>(mResourcePtr)) {
				return aDescriptorSet.store_buffer_info(mLayoutBinding.binding, std::get<const generic_buffer_t*>(mResourcePtr)->descriptor_info());
			}
			if (std::holds_alternative<const uniform_buffer_t*>(mResourcePtr)) {
				return aDescriptorSet.store_buffer_info(mLayoutBinding.binding, std::get<const uniform_buffer_t*>(mResourcePtr)->descriptor_info());
			}
			if (std::holds_alternative<const uniform_texel_buffer_t*>(mResourcePtr)) {
				return aDescriptorSet.store_buffer_info(mLayoutBinding.binding, std::get<const uniform_texel_buffer_t*>(mResourcePtr)->descriptor_info());
			}
			if (std::holds_alternative<const storage_buffer_t*>(mResourcePtr)) {
				return aDescriptorSet.store_buffer_info(mLayoutBinding.binding, std::get<const storage_buffer_t*>(mResourcePtr)->descriptor_info());
			}
			if (std::holds_alternative<const storage_texel_buffer_t*>(mResourcePtr)) {
				return aDescriptorSet.store_buffer_info(mLayoutBinding.binding, std::get<const storage_texel_buffer_t*>(mResourcePtr)->descriptor_info());
			}
			if (std::holds_alternative<const vertex_buffer_t*>(mResourcePtr)) {
				return aDescriptorSet.store_buffer_info(mLayoutBinding.binding, std::get<const vertex_buffer_t*>(mResourcePtr)->descriptor_info());
			}
			if (std::holds_alternative<const index_buffer_t*>(mResourcePtr)) {
				return aDescriptorSet.store_buffer_info(mLayoutBinding.binding, std::get<const index_buffer_t*>(mResourcePtr)->descriptor_info());
			}
			if (std::holds_alternative<const instance_buffer_t*>(mResourcePtr)) {
				return aDescriptorSet.store_buffer_info(mLayoutBinding.binding, std::get<const instance_buffer_t*>(mResourcePtr)->descriptor_info());
			}
			
			if (std::holds_alternative<const top_level_acceleration_structure_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const image_view_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const image_view_as_input_attachment*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const image_view_as_storage_image*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const sampler_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const image_sampler_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const buffer_view_t*>(mResourcePtr)) { return nullptr; }

			if (std::holds_alternative<std::vector<const generic_buffer_t*>>(mResourcePtr)) {
				return aDescriptorSet.store_buffer_infos(mLayoutBinding.binding, gather_buffer_infos(std::get<std::vector<const generic_buffer_t*>>(mResourcePtr)));
			}
			if (std::holds_alternative<std::vector<const uniform_buffer_t*>>(mResourcePtr)) { 
				return aDescriptorSet.store_buffer_infos(mLayoutBinding.binding, gather_buffer_infos(std::get<std::vector<const uniform_buffer_t*>>(mResourcePtr)));
			}
			if (std::holds_alternative<std::vector<const uniform_texel_buffer_t*>>(mResourcePtr)) {
				return aDescriptorSet.store_buffer_infos(mLayoutBinding.binding, gather_buffer_infos(std::get<std::vector<const uniform_texel_buffer_t*>>(mResourcePtr)));
			}
			if (std::holds_alternative<std::vector<const storage_buffer_t*>>(mResourcePtr)) {
				return aDescriptorSet.store_buffer_infos(mLayoutBinding.binding, gather_buffer_infos(std::get<std::vector<const storage_buffer_t*>>(mResourcePtr)));
			}
			if (std::holds_alternative<std::vector<const storage_texel_buffer_t*>>(mResourcePtr)) {
				return aDescriptorSet.store_buffer_infos(mLayoutBinding.binding, gather_buffer_infos(std::get<std::vector<const storage_texel_buffer_t*>>(mResourcePtr)));
			}
			if (std::holds_alternative<std::vector<const vertex_buffer_t*>>(mResourcePtr)) {
				return aDescriptorSet.store_buffer_infos(mLayoutBinding.binding, gather_buffer_infos(std::get<std::vector<const vertex_buffer_t*>>(mResourcePtr)));
			}
			if (std::holds_alternative<std::vector<const index_buffer_t*>>(mResourcePtr)) {
				return aDescriptorSet.store_buffer_infos(mLayoutBinding.binding, gather_buffer_infos(std::get<std::vector<const index_buffer_t*>>(mResourcePtr)));
			}
			if (std::holds_alternative<std::vector<const instance_buffer_t*>>(mResourcePtr)) {
				return aDescriptorSet.store_buffer_infos(mLayoutBinding.binding, gather_buffer_infos(std::get<std::vector<const instance_buffer_t*>>(mResourcePtr)));
			}

			if (std::holds_alternative<std::vector<const top_level_acceleration_structure_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const image_view_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const image_view_as_input_attachment*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const image_view_as_storage_image*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const sampler_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const image_sampler_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const buffer_view_t*>>(mResourcePtr)) { return nullptr; }
			
			throw cgb::runtime_error("Some holds_alternative calls are not implemented.");
		}

		const void* next_pointer(descriptor_set& aDescriptorSet) const
		{
			if (std::holds_alternative<const generic_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const uniform_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const uniform_texel_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const storage_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const storage_texel_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const vertex_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const index_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const instance_buffer_t*>(mResourcePtr)) { return nullptr; }
			
			if (std::holds_alternative<const top_level_acceleration_structure_t*>(mResourcePtr)) {
				return aDescriptorSet.store_acceleration_structure_info(mLayoutBinding.binding, std::get<const top_level_acceleration_structure_t*>(mResourcePtr)->descriptor_info());
			}
			
			if (std::holds_alternative<const image_view_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const image_view_as_input_attachment*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const image_view_as_storage_image*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const sampler_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const image_sampler_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const buffer_view_t*>(mResourcePtr)) { return nullptr; }

			if (std::holds_alternative<std::vector<const generic_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const uniform_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const uniform_texel_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const storage_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const storage_texel_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const vertex_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const index_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const instance_buffer_t*>>(mResourcePtr)) { return nullptr; }

			if (std::holds_alternative<std::vector<const top_level_acceleration_structure_t*>>(mResourcePtr)) {
				return aDescriptorSet.store_acceleration_structure_infos(mLayoutBinding.binding, gather_acceleration_structure_infos(std::get<std::vector<const top_level_acceleration_structure_t*>>(mResourcePtr)));
			}

			if (std::holds_alternative<std::vector<const image_view_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const image_view_as_input_attachment*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const image_view_as_storage_image*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const sampler_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const image_sampler_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const buffer_view_t*>>(mResourcePtr)) { return nullptr; }
			
			throw cgb::runtime_error("Some holds_alternative calls are not implemented.");
		}

		const vk::BufferView* texel_buffer_view_info(descriptor_set& aDescriptorSet) const
		{
			if (std::holds_alternative<const generic_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const uniform_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const uniform_texel_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const storage_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const storage_texel_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const vertex_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const index_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const instance_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const top_level_acceleration_structure_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const image_view_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const image_view_as_input_attachment*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const image_view_as_storage_image*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const sampler_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<const image_sampler_t*>(mResourcePtr)) { return nullptr; }
			
			if (std::holds_alternative<const buffer_view_t*>(mResourcePtr)) {
				return aDescriptorSet.store_buffer_view(mLayoutBinding.binding, std::get<const buffer_view_t*>(mResourcePtr)->view_handle());
			}

			if (std::holds_alternative<std::vector<const generic_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const uniform_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const uniform_texel_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const storage_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const storage_texel_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const vertex_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const index_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const instance_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const top_level_acceleration_structure_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const image_view_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const image_view_as_input_attachment*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const image_view_as_storage_image*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const sampler_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<const image_sampler_t*>>(mResourcePtr)) { return nullptr; }
			
			if (std::holds_alternative<std::vector<const buffer_view_t*>>(mResourcePtr)) {
				return aDescriptorSet.store_buffer_views(mLayoutBinding.binding, gather_buffer_views(std::get<std::vector<const buffer_view_t*>>(mResourcePtr)));
			}
			
			throw cgb::runtime_error("Some holds_alternative calls are not implemented.");
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
