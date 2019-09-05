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
			generic_buffer_t*,
			uniform_buffer_t*,
			uniform_texel_buffer_t*,
			storage_buffer_t*,
			storage_texel_buffer_t*,
			vertex_buffer_t*,
			index_buffer_t*,
			instance_buffer_t*,
			top_level_acceleration_structure_t*,
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
			std::vector<top_level_acceleration_structure_t*>,
			std::vector<image_view_t*>,
			std::vector<sampler_t*>,
			std::vector<image_sampler_t*>
		> mResourcePtr;

		mutable std::vector<vk::DescriptorImageInfo> mThisIsProbablyAHackForImageInfos;
		mutable std::vector<vk::DescriptorBufferInfo> mThisIsProbablyAHackForBufferInfos;
		mutable std::vector<void*> mThisIsProbablyAHackForPNexts;
		mutable std::vector<vk::BufferView> mThisIsProbablyAHackForBufferViews;

		template <typename T>
		const vk::DescriptorImageInfo* fill_this_is_probably_a_hack_for_image_infos(const std::vector<T*>& vec) const
		{
			mThisIsProbablyAHackForImageInfos.clear();
			for (auto& v : vec) {
				mThisIsProbablyAHackForImageInfos.push_back(v->descriptor_info());
			}
			return mThisIsProbablyAHackForImageInfos.data();
		}

		template <typename T>
		const vk::DescriptorBufferInfo* fill_this_is_probably_a_hack_for_buffer_infos(const std::vector<T*>& vec) const
		{
			mThisIsProbablyAHackForBufferInfos.clear();
			for (auto& v : vec) {
				mThisIsProbablyAHackForBufferInfos.push_back(v->descriptor_info());
			}
			return mThisIsProbablyAHackForBufferInfos.data();
		}

		template <typename T>
		const void* fill_this_is_probably_a_hack_for_pnexts(const std::vector<T*>& vec) const
		{
			mThisIsProbablyAHackForPNexts.clear();
			for (auto& v : vec) {
				mThisIsProbablyAHackForPNexts.push_back(const_cast<void*>(static_cast<const void*>(&v->descriptor_info())));
			}
			return mThisIsProbablyAHackForPNexts.data();
		}

		template <typename T>
		const vk::BufferView* fill_this_is_probably_a_hack_for_buffer_views(const std::vector<T*>& vec) const
		{
			mThisIsProbablyAHackForBufferViews.clear();
			for (auto& v : vec) {
				mThisIsProbablyAHackForBufferViews.push_back(v->descriptor_info());
			}
			return mThisIsProbablyAHackForBufferViews.data();
		}

		uint32_t descriptor_count() const
		{
			if (std::holds_alternative<std::vector<generic_buffer_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<generic_buffer_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<uniform_buffer_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<uniform_buffer_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<uniform_texel_buffer_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<uniform_texel_buffer_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<storage_buffer_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<storage_buffer_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<storage_texel_buffer_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<storage_texel_buffer_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<vertex_buffer_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<vertex_buffer_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<index_buffer_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<index_buffer_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<instance_buffer_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<instance_buffer_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<top_level_acceleration_structure_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<top_level_acceleration_structure_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<image_view_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<image_view_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<sampler_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<sampler_t*>>(mResourcePtr).size()); }
			if (std::holds_alternative<std::vector<image_sampler_t*>>(mResourcePtr)) { return static_cast<uint32_t>(std::get<std::vector<image_sampler_t*>>(mResourcePtr).size()); }
			return 1u;
		}

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
			if (std::holds_alternative<top_level_acceleration_structure_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<image_view_t*>(mResourcePtr)) { return &std::get<image_view_t*>(mResourcePtr)->descriptor_info(); }
			if (std::holds_alternative<sampler_t*>(mResourcePtr)) { return &std::get<sampler_t*>(mResourcePtr)->descriptor_info(); }
			if (std::holds_alternative<image_sampler_t*>(mResourcePtr)) { return &std::get<image_sampler_t*>(mResourcePtr)->descriptor_info(); }

			if (std::holds_alternative<std::vector<generic_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<uniform_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<uniform_texel_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<storage_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<storage_texel_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<vertex_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<index_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<instance_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<top_level_acceleration_structure_t*>>(mResourcePtr)) { return nullptr; }

			if (std::holds_alternative<std::vector<image_view_t*>>(mResourcePtr)) { // TODO: OMG, I don't know... shouldn't this be handled somehow differently??
				return fill_this_is_probably_a_hack_for_image_infos(std::get<std::vector<image_view_t*>>(mResourcePtr));
			}
			if (std::holds_alternative<std::vector<sampler_t*>>(mResourcePtr)) { // TODO: OMG, I don't know... shouldn't this be handled somehow differently??
				return fill_this_is_probably_a_hack_for_image_infos(std::get<std::vector<sampler_t*>>(mResourcePtr));
			}
			if (std::holds_alternative<std::vector<image_sampler_t*>>(mResourcePtr)) { // TODO: OMG, I don't know... shouldn't this be handled somehow differently??
				return fill_this_is_probably_a_hack_for_image_infos(std::get<std::vector<image_sampler_t*>>(mResourcePtr));
			}
			
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
			if (std::holds_alternative<instance_buffer_t*>(mResourcePtr)) { return &std::get<instance_buffer_t*>(mResourcePtr)->descriptor_info(); }
			if (std::holds_alternative<top_level_acceleration_structure_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<image_view_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<sampler_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<image_sampler_t*>(mResourcePtr)) { return nullptr; }

			if (std::holds_alternative<std::vector<generic_buffer_t*>>(mResourcePtr)) { // TODO: OMG, I don't know... shouldn't this be handled somehow differently??
				return fill_this_is_probably_a_hack_for_buffer_infos(std::get<std::vector<generic_buffer_t*>>(mResourcePtr));
			}
			if (std::holds_alternative<std::vector<uniform_buffer_t*>>(mResourcePtr)) { // TODO: OMG, I don't know... shouldn't this be handled somehow differently??
				return fill_this_is_probably_a_hack_for_buffer_infos(std::get<std::vector<uniform_buffer_t*>>(mResourcePtr));
			}
			if (std::holds_alternative<std::vector<uniform_texel_buffer_t*>>(mResourcePtr)) { // TODO: OMG, I don't know... shouldn't this be handled somehow differently??
				return fill_this_is_probably_a_hack_for_buffer_infos(std::get<std::vector<uniform_texel_buffer_t*>>(mResourcePtr));
			}
			if (std::holds_alternative<std::vector<storage_buffer_t*>>(mResourcePtr)) { // TODO: OMG, I don't know... shouldn't this be handled somehow differently??
				return fill_this_is_probably_a_hack_for_buffer_infos(std::get<std::vector<storage_buffer_t*>>(mResourcePtr));
			}
			if (std::holds_alternative<std::vector<storage_texel_buffer_t*>>(mResourcePtr)) { // TODO: OMG, I don't know... shouldn't this be handled somehow differently??
				return fill_this_is_probably_a_hack_for_buffer_infos(std::get<std::vector<storage_texel_buffer_t*>>(mResourcePtr));
			}
			if (std::holds_alternative<std::vector<vertex_buffer_t*>>(mResourcePtr)) { // TODO: OMG, I don't know... shouldn't this be handled somehow differently??
				return fill_this_is_probably_a_hack_for_buffer_infos(std::get<std::vector<vertex_buffer_t*>>(mResourcePtr));
			}
			if (std::holds_alternative<std::vector<index_buffer_t*>>(mResourcePtr)) { // TODO: OMG, I don't know... shouldn't this be handled somehow differently??
				return fill_this_is_probably_a_hack_for_buffer_infos(std::get<std::vector<index_buffer_t*>>(mResourcePtr));
			}
			if (std::holds_alternative<std::vector<instance_buffer_t*>>(mResourcePtr)) { // TODO: OMG, I don't know... shouldn't this be handled somehow differently??
				return fill_this_is_probably_a_hack_for_buffer_infos(std::get<std::vector<instance_buffer_t*>>(mResourcePtr));
			}

			if (std::holds_alternative<std::vector<top_level_acceleration_structure_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<image_view_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<sampler_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<image_sampler_t*>>(mResourcePtr)) { return nullptr; }
			
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
			if (std::holds_alternative<top_level_acceleration_structure_t*>(mResourcePtr)) { return &std::get<top_level_acceleration_structure_t*>(mResourcePtr)->descriptor_info(); }
			if (std::holds_alternative<image_view_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<sampler_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<image_sampler_t*>(mResourcePtr)) { return nullptr; }

			if (std::holds_alternative<std::vector<generic_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<uniform_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<uniform_texel_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<storage_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<storage_texel_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<vertex_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<index_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<instance_buffer_t*>>(mResourcePtr)) { return nullptr; }

			if (std::holds_alternative<std::vector<top_level_acceleration_structure_t*>>(mResourcePtr)) {// TODO: OMG, I don't know... shouldn't this be handled somehow differently??
				return fill_this_is_probably_a_hack_for_pnexts(std::get<std::vector<top_level_acceleration_structure_t*>>(mResourcePtr));
			}

			if (std::holds_alternative<std::vector<image_view_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<sampler_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<image_sampler_t*>>(mResourcePtr)) { return nullptr; }
			
			throw std::runtime_error("Some holds_alternative calls are not implemented.");
		}

		const vk::BufferView* texel_buffer_view_info() const
		{
			if (std::holds_alternative<generic_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<uniform_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<uniform_texel_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<storage_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<storage_texel_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<vertex_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<index_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<instance_buffer_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<top_level_acceleration_structure_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<image_view_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<sampler_t*>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<image_sampler_t*>(mResourcePtr)) { return nullptr; }

			if (std::holds_alternative<std::vector<generic_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<uniform_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<uniform_texel_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<storage_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<storage_texel_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<vertex_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<index_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<instance_buffer_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<top_level_acceleration_structure_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<image_view_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<sampler_t*>>(mResourcePtr)) { return nullptr; }
			if (std::holds_alternative<std::vector<image_sampler_t*>>(mResourcePtr)) { return nullptr; }
			
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
