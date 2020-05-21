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
		const auto& all_sets() { return mLayouts; }
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
		const auto handle() const { return mDescriptorSet; }
		const auto set_id() const { return mSetId; }

		const auto* store_image_infos(uint32_t aBindingId, std::vector<vk::DescriptorImageInfo> aStoredImageInfos)
		{
			auto back = mStoredImageInfos.emplace_back(aBindingId, std::move(aStoredImageInfos));
			return std::get<std::vector<vk::DescriptorImageInfo>>(back).data();
		}
		
		const auto* store_buffer_infos(uint32_t aBindingId, std::vector<vk::DescriptorBufferInfo> aStoredBufferInfos)
		{
			auto back = mStoredBufferInfos.emplace_back(aBindingId, std::move(aStoredBufferInfos));
			return std::get<std::vector<vk::DescriptorBufferInfo>>(back).data();
		}
		
		const vk::WriteDescriptorSetAccelerationStructureKHR* store_acceleration_structure_infos(uint32_t aBindingId, std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> aWriteAccelerationStructureInfos)
		{
			// Accumulate all into ONE! (At least I think "This is the way.")
			std::tuple<vk::WriteDescriptorSetAccelerationStructureKHR, std::vector<vk::AccelerationStructureKHR>> oneAndOnlyWrite;

			for (auto& wasi : aWriteAccelerationStructureInfos) {
				for (uint32_t i = 0u; i < wasi.accelerationStructureCount; ++i) {
					std::get<std::vector<vk::AccelerationStructureKHR>>(oneAndOnlyWrite).push_back(wasi.pAccelerationStructures[i]);
				}
			}

			std::get<vk::WriteDescriptorSetAccelerationStructureKHR>(oneAndOnlyWrite).accelerationStructureCount = static_cast<uint32_t>(std::get<std::vector<vk::AccelerationStructureKHR>>(oneAndOnlyWrite).size());
			
			auto back = mStoredAccelerationStructureWrites.emplace_back(aBindingId, std::move(oneAndOnlyWrite));
			return &std::get<vk::WriteDescriptorSetAccelerationStructureKHR>(std::get<1>(back));
		}

		const auto* store_buffer_views(uint32_t aBindingId, std::vector<vk::BufferView> aStoredBufferViews)
		{
			auto back = mStoredBufferViews.emplace_back(aBindingId, std::move(aStoredBufferViews));
			return std::get<std::vector<vk::BufferView>>(back).data();
		}

		const auto* store_image_info(uint32_t aBindingId, const vk::DescriptorImageInfo& aStoredImageInfo)
		{
			auto back = mStoredImageInfos.emplace_back(aBindingId, cgb::make_vector( aStoredImageInfo ));
			return std::get<std::vector<vk::DescriptorImageInfo>>(back).data();
		}
		
		const auto* store_buffer_info(uint32_t aBindingId, const vk::DescriptorBufferInfo& aStoredBufferInfo)
		{
			auto back = mStoredBufferInfos.emplace_back(aBindingId, cgb::make_vector( aStoredBufferInfo ));
			return std::get<std::vector<vk::DescriptorBufferInfo>>(back).data();
		}
		
		const vk::WriteDescriptorSetAccelerationStructureKHR* store_acceleration_structure_info(uint32_t aBindingId, const vk::WriteDescriptorSetAccelerationStructureKHR& aWriteAccelerationStructureInfo)
		{
			std::vector<vk::AccelerationStructureKHR> accStructureHandles;
			for (uint32_t i = 0u; i < aWriteAccelerationStructureInfo.accelerationStructureCount; ++i) {
				accStructureHandles.push_back(aWriteAccelerationStructureInfo.pAccelerationStructures[i]);
			}

			auto theWrite = std::make_tuple<vk::WriteDescriptorSetAccelerationStructureKHR, std::vector<vk::AccelerationStructureKHR>>(
				vk::WriteDescriptorSetAccelerationStructureKHR{aWriteAccelerationStructureInfo}, std::move(accStructureHandles)
			);
			
			auto back = mStoredAccelerationStructureWrites.emplace_back(aBindingId, std::move(theWrite));
			return &std::get<vk::WriteDescriptorSetAccelerationStructureKHR>(std::get<1>(back));
		}

		const auto* store_buffer_view(uint32_t aBindingId, const vk::BufferView& aStoredBufferView)
		{
			auto back = mStoredBufferViews.emplace_back(aBindingId, cgb::make_vector( aStoredBufferView ));
			return std::get<std::vector<vk::BufferView>>(back).data();
		}

		void update_data_pointers();
		
		template <typename It>
		static descriptor_set prepare(It begin, It end)
		{
			descriptor_set result;
			result.mSetId = begin->mSetId;
			
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
					b.descriptor_image_info(result),
					b.descriptor_buffer_info(result),
					b.texel_buffer_view_info(result)
				);
				result.mOrderedDescriptorDataWrites.back().setPNext(b.next_pointer(result));
				
				++it;
			}

			result.update_data_pointers();
			//result.mTracker.setTrackee(result);
			return result;
		}

		void link_to_handle_and_pool(vk::DescriptorSet aHandle, std::shared_ptr<descriptor_pool> aPool);
		void write_descriptors();
		
		static std::vector<const descriptor_set*> get_or_create(std::initializer_list<binding_data> aBindings, descriptor_cache_interface* aCache = nullptr);

	private:
		std::vector<vk::WriteDescriptorSet> mOrderedDescriptorDataWrites;
		std::shared_ptr<descriptor_pool> mPool;
		vk::DescriptorSet mDescriptorSet;
		// TODO: Are there cases where vk::UniqueDescriptorSet would be beneficial? Right now, the pool cleans up all the descriptor sets.
		uint32_t mSetId;
		std::vector<std::tuple<uint32_t, std::vector<vk::DescriptorImageInfo>>> mStoredImageInfos;
		std::vector<std::tuple<uint32_t, std::vector<vk::DescriptorBufferInfo>>> mStoredBufferInfos;
		std::vector<std::tuple<uint32_t, std::vector<vk::BufferView>>> mStoredBufferViews;
		std::vector<std::tuple<uint32_t, std::tuple<vk::WriteDescriptorSetAccelerationStructureKHR, std::vector<vk::AccelerationStructureKHR>>>> mStoredAccelerationStructureWrites;
		//cgb::context_tracker<descriptor_set> mTracker;
	};

	static bool operator ==(const descriptor_set& left, const descriptor_set& right) {
		const auto n = left.mOrderedDescriptorDataWrites.size();
		if (n != right.mOrderedDescriptorDataWrites.size()) {
			return false;
		}
		for (size_t i = 0; i < n; ++i) {
			if (left.mOrderedDescriptorDataWrites[i].dstBinding			!= right.mOrderedDescriptorDataWrites[i].dstBinding			)			{ return false; }
			if (left.mOrderedDescriptorDataWrites[i].dstArrayElement	!= right.mOrderedDescriptorDataWrites[i].dstArrayElement	)			{ return false; }
			if (left.mOrderedDescriptorDataWrites[i].descriptorCount	!= right.mOrderedDescriptorDataWrites[i].descriptorCount	)			{ return false; }
			if (left.mOrderedDescriptorDataWrites[i].descriptorType		!= right.mOrderedDescriptorDataWrites[i].descriptorType		)			{ return false; }
			if (nullptr != left.mOrderedDescriptorDataWrites[i].pImageInfo) {
				if (nullptr == right.mOrderedDescriptorDataWrites[i].pImageInfo)																{ return false; }
				for (size_t j = 0; j < left.mOrderedDescriptorDataWrites[i].descriptorCount; ++j) {
					if (left.mOrderedDescriptorDataWrites[i].pImageInfo[j] != right.mOrderedDescriptorDataWrites[i].pImageInfo[j])				{ return false; }
				}
			}
			if (nullptr != left.mOrderedDescriptorDataWrites[i].pBufferInfo) {
				if (nullptr == right.mOrderedDescriptorDataWrites[i].pBufferInfo)																{ return false; }
				for (size_t j = 0; j < left.mOrderedDescriptorDataWrites[i].descriptorCount; ++j) {
					if (left.mOrderedDescriptorDataWrites[i].pBufferInfo[j] != right.mOrderedDescriptorDataWrites[i].pBufferInfo[j])			{ return false; }
				}
			}
			if (nullptr != left.mOrderedDescriptorDataWrites[i].pTexelBufferView) {
				if (nullptr == right.mOrderedDescriptorDataWrites[i].pTexelBufferView)															{ return false; }
				for (size_t j = 0; j < left.mOrderedDescriptorDataWrites[i].descriptorCount; ++j) {
					if (left.mOrderedDescriptorDataWrites[i].pTexelBufferView[j] != right.mOrderedDescriptorDataWrites[i].pTexelBufferView[j])	{ return false; }
				}
			}
			if (nullptr != left.mOrderedDescriptorDataWrites[i].pNext) {
				if (nullptr == right.mOrderedDescriptorDataWrites[i].pNext)																		{ return false; }
				if (left.mOrderedDescriptorDataWrites[i].descriptorType == vk::DescriptorType::eAccelerationStructureKHR) {
					const auto* asInfoLeft = reinterpret_cast<const VkWriteDescriptorSetAccelerationStructureKHR*>(left.mOrderedDescriptorDataWrites[i].pNext);
					const auto* asInfoRight = reinterpret_cast<const VkWriteDescriptorSetAccelerationStructureKHR*>(right.mOrderedDescriptorDataWrites[i].pNext);
					if (asInfoLeft->accelerationStructureCount != asInfoRight->accelerationStructureCount)										{ return false; }
					for (size_t j = 0; j < asInfoLeft->accelerationStructureCount; ++j) {
						if (asInfoLeft->pAccelerationStructures[j] != asInfoRight->pAccelerationStructures[j])									{ return false; }
					}
				}
			}
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
				cgb::hash_combine(h, binding.binding, binding.descriptorType, binding.descriptorCount, static_cast<VkShaderStageFlags>(binding.stageFlags), binding.pImmutableSamplers);
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
				cgb::hash_combine(h, w.dstBinding, w.dstArrayElement, w.descriptorCount, w.descriptorType);
				// Dont compute a too expensive hash => only take the first elements, each:
				if (nullptr != w.pImageInfo && w.descriptorCount > 0) {
					cgb::hash_combine(h, static_cast<VkSampler>(w.pImageInfo[0].sampler), static_cast<VkImageView>(w.pImageInfo[0].imageView), static_cast<VkImageLayout>(w.pImageInfo[0].imageLayout));
				}
				if (nullptr != w.pBufferInfo && w.descriptorCount > 0) {
					cgb::hash_combine(h, static_cast<VkBuffer>(w.pBufferInfo[0].buffer), w.pBufferInfo[0].offset, w.pBufferInfo[0].range);
				}
				if (nullptr != w.pTexelBufferView && w.descriptorCount > 0) {
					cgb::hash_combine(h, static_cast<VkBufferView>(w.pTexelBufferView[0]));
				}
				if (nullptr != w.pNext) {
					if (w.descriptorType == vk::DescriptorType::eAccelerationStructureKHR) {
						const auto* asInfo = reinterpret_cast<const VkWriteDescriptorSetAccelerationStructureKHR*>(w.pNext);
						cgb::hash_combine(h, asInfo->accelerationStructureCount);
						if (asInfo->accelerationStructureCount > 0) {
							cgb::hash_combine(h, static_cast<VkAccelerationStructureKHR>(asInfo->pAccelerationStructures[0]));
						}
					}
					else {
						cgb::hash_combine(h, nullptr != w.pNext);
					}
				}
				// operator== will test for exact equality.
			}
			return h;
		}
	};

}
