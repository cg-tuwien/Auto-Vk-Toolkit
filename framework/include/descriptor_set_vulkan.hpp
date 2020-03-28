#pragma once

namespace cgb
{
	/**	Represents a descriptor set layout, contains also the bindings
	 *	stored in binding-order, and the accumulated descriptor counts
	 *	per bindings which is information that can be used for configuring
	 *	descriptor pools.
	 */
	class descriptor_set_layout
	{
		friend static bool operator ==(const descriptor_set_layout& left, const descriptor_set_layout& right);
		friend static bool operator !=(const descriptor_set_layout& left, const descriptor_set_layout& right);
		friend struct std::hash<cgb::descriptor_set_layout>;
		
	public:
		descriptor_set_layout() = default;
		descriptor_set_layout(const descriptor_set_layout&) = delete;
		descriptor_set_layout(descriptor_set_layout&&) noexcept = default;
		descriptor_set_layout& operator=(const descriptor_set_layout&) = delete;
		descriptor_set_layout& operator=(descriptor_set_layout&&) noexcept = default;
		~descriptor_set_layout() = default;

		const auto& required_pool_sizes() const { return mBindingRequirements; }
		auto number_of_bindings() const { return mOrderedBindings.size(); }
		const auto& binding_at(size_t i) const { return mOrderedBindings[i]; }
		auto handle() const { return mLayout.get(); }

		template <typename It>
		static descriptor_set_layout prepare(It begin, It end)
		{
			// Put elements from initializer list into vector of ORDERED bindings:
			descriptor_set_layout result;
			
			It it = begin;
			while (it != end) {
				const binding_data& b = *it;
				assert(begin->mSetId == b.mSetId);

				{ // Assemble the mBindingRequirements member:
				  // ordered by descriptor type
					auto entry = vk::DescriptorPoolSize{}
						.setType(b.mLayoutBinding.descriptorType)
						.setDescriptorCount(b.mLayoutBinding.descriptorCount);
					// find position where to insert in vector
					auto pos = std::lower_bound(std::begin(result.mBindingRequirements), std::end(result.mBindingRequirements), 
						entry,
						[](const vk::DescriptorPoolSize& first, const vk::DescriptorPoolSize& second) -> bool {
							using EnumType = std::underlying_type<vk::DescriptorType>::type;
							return static_cast<EnumType>(first.type) < static_cast<EnumType>(second.type);
						});
					// Maybe accumulate
					if (pos != std::end(result.mBindingRequirements) && pos->type == entry.type) {
						pos->descriptorCount += entry.descriptorCount;
					}
					else {
						result.mBindingRequirements.insert(pos, entry);
					}
				}

				// Store the ordered binding:
				assert((it+1) == end || b.mLayoutBinding.binding != (it+1)->mLayoutBinding.binding);
				assert((it+1) == end || b.mLayoutBinding.binding < (it+1)->mLayoutBinding.binding);
				result.mOrderedBindings.push_back(b.mLayoutBinding);
				
				it++;
			}

			// Preparation is done
			return result;
		}

		static descriptor_set_layout prepare(std::vector<binding_data> pBindings);

		void allocate();

	private:
		std::vector<vk::DescriptorPoolSize> mBindingRequirements;
		std::vector<vk::DescriptorSetLayoutBinding> mOrderedBindings;
		vk::UniqueDescriptorSetLayout mLayout;
	};

	static bool operator ==(const descriptor_set_layout& left, const descriptor_set_layout& right) {
		const auto n = left.mOrderedBindings.size();
		if (n != right.mOrderedBindings.size()) {
			return false;
		}
		for (size_t i = 0; i < n; ++i) {
			if (left.mOrderedBindings[i] != right.mOrderedBindings[i]) {
				return false;
			}
		}
		return true;
	}

	static bool operator !=(const descriptor_set_layout& left, const descriptor_set_layout& right) {
		return !(left == right);
	}
	

	/** Basically a vector of descriptor_set_layout instances */
	class set_of_descriptor_set_layouts
	{
	public:
		set_of_descriptor_set_layouts() = default;
		set_of_descriptor_set_layouts(const set_of_descriptor_set_layouts&) = delete;
		set_of_descriptor_set_layouts(set_of_descriptor_set_layouts&&) noexcept = default;
		set_of_descriptor_set_layouts& operator=(const set_of_descriptor_set_layouts&) = delete;
		set_of_descriptor_set_layouts& operator=(set_of_descriptor_set_layouts&&) noexcept = default;
		~set_of_descriptor_set_layouts() = default;

		static set_of_descriptor_set_layouts prepare(std::vector<binding_data> pBindings);

		void allocate_all();

		uint32_t number_of_sets() const { return static_cast<uint32_t>(mLayouts.size()); }
		const auto& set_at(uint32_t pIndex) const { return mLayouts[pIndex]; }
		auto& set_at(uint32_t pIndex) { return mLayouts[pIndex]; }
		uint32_t set_index_for_set_id(uint32_t pSetId) const { return pSetId - mFirstSetId; }
		const auto& set_for_set_id(uint32_t pSetId) const { return set_at(set_index_for_set_id(pSetId)); }
		const auto& required_pool_sizes() const { return mBindingRequirements; }
		std::vector<vk::DescriptorSetLayout> layout_handles() const;

	private:
		std::vector<vk::DescriptorPoolSize> mBindingRequirements;
		uint32_t mFirstSetId; // Set-Id of the first set, all the following sets have consecutive ids
		std::vector<descriptor_set_layout> mLayouts;
	};
	
	/** Descriptor set */
	class descriptor_set
	{
		friend static bool operator ==(const descriptor_set& left, const descriptor_set& right);
		friend static bool operator !=(const descriptor_set& left, const descriptor_set& right);
		friend struct std::hash<cgb::descriptor_set>;
		
	public:
		descriptor_set() = default;
		descriptor_set(descriptor_set&&) noexcept = default;
		descriptor_set(const descriptor_set&) = delete;
		descriptor_set& operator=(descriptor_set&&) noexcept = default;
		descriptor_set& operator=(const descriptor_set&) = delete;
		~descriptor_set() = default;

		auto number_of_writes() const { return mOrderedDescriptorDataWrites.size(); }
		const auto& write_at(size_t i) const { return mOrderedDescriptorDataWrites[i]; }
		const auto* pool() const { return static_cast<bool>(mPool) ? mPool.get() : nullptr; }
		const auto handle() const { return mDescriptorSet.get(); }

		template <typename It>
		static descriptor_set prepare(It begin, It end)
		{
			descriptor_set result;

			It it = begin;
			while (it != end) {
				const binding_data& b = *it;
				assert(begin->mSetId == b.mSetId);

				assert((it+1) == end || b.mLayoutBinding.binding != (it+1)->mLayoutBinding.binding);
				assert((it+1) == end || b.mLayoutBinding.binding < (it+1)->mLayoutBinding.binding);

				result.mOrderedDescriptorDataWrites.emplace_back(
					vk::DescriptorSet{}, // To be set before actually writing
					b.mLayoutBinding.binding,
					0u, // TODO: Maybe support other array offsets
					b.descriptor_count(),
					b.mLayoutBinding.descriptorType,
					b.descriptor_image_info(),
					b.descriptor_buffer_info(),
					b.texel_buffer_view_info()
				);
				
				it++;
			}

			return result;
		}

		void link_to_handle_and_pool(vk::UniqueDescriptorSet aHandle, std::shared_ptr<descriptor_pool> aPool);
		void write_descriptors();
		
		std::vector<const descriptor_set*> create(std::initializer_list<binding_data> aBindings, descriptor_cache_interface* aCache = nullptr);

	private:
		std::vector<vk::WriteDescriptorSet> mOrderedDescriptorDataWrites;
		std::shared_ptr<descriptor_pool> mPool;
		vk::UniqueDescriptorSet mDescriptorSet;
	};

	static bool operator ==(const descriptor_set& left, const descriptor_set& right) {
		const auto n = left.mOrderedDescriptorDataWrites.size();
		if (n != right.mOrderedDescriptorDataWrites.size()) {
			return false;
		}
		for (size_t i = 0; i < n; ++i) {
			if (left.mOrderedDescriptorDataWrites[i].dstBinding	   != right.mOrderedDescriptorDataWrites[i].dstBinding			) { return false; }
			if (left.mOrderedDescriptorDataWrites[i].dstArrayElement  != right.mOrderedDescriptorDataWrites[i].dstArrayElement	) { return false; }
			if (left.mOrderedDescriptorDataWrites[i].descriptorCount  != right.mOrderedDescriptorDataWrites[i].descriptorCount	) { return false; }
			if (left.mOrderedDescriptorDataWrites[i].descriptorType   != right.mOrderedDescriptorDataWrites[i].descriptorType		) { return false; }
			if (left.mOrderedDescriptorDataWrites[i].pImageInfo 	   != right.mOrderedDescriptorDataWrites[i].pImageInfo 		) { return false; } // TODO: Compare pointers or handles?
			if (left.mOrderedDescriptorDataWrites[i].pBufferInfo	   != right.mOrderedDescriptorDataWrites[i].pBufferInfo		) { return false; } // TODO: Compare pointers or handles?
			if (left.mOrderedDescriptorDataWrites[i].pTexelBufferView != right.mOrderedDescriptorDataWrites[i].pTexelBufferView	) { return false; } // TODO: Compare pointers or handles?
		}
		return true;
	}

	static bool operator !=(const descriptor_set& left, const descriptor_set& right) {
		return !(left == right);
	}
}

namespace std
{
	template<> struct hash<cgb::descriptor_set_layout>
	{
		std::size_t operator()(cgb::descriptor_set_layout const& o) const noexcept
		{
			std::size_t h = 0;
			for(auto& binding : o.mOrderedBindings)
			{
				cgb::hash_combine(h, binding.binding, binding.descriptorType, binding.descriptorCount, binding.stageFlags, binding.pImmutableSamplers);
			}
			return h;
		}
	};

	template<> struct hash<cgb::descriptor_set>
	{
		std::size_t operator()(cgb::descriptor_set const& o) const noexcept
		{
			std::size_t h = 0;
			for(auto& w : o.mOrderedDescriptorDataWrites)
			{
				cgb::hash_combine(h, w.dstBinding, w.dstArrayElement, w.descriptorCount, w.descriptorType, w.pImageInfo, w.pBufferInfo, w.pTexelBufferView);
			}
			return h;
		}
	};

}
