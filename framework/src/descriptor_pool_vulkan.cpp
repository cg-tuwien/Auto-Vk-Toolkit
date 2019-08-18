namespace cgb
{
	descriptor_alloc_request descriptor_alloc_request::create(std::initializer_list<std::reference_wrapper<descriptor_set_layout>> pLayouts)
	{
		descriptor_alloc_request result;

		for (auto& layout : pLayouts) {
			// Store the "reference" and hope that it will not get out of scope somewhen
			result.mToAlloc.push_back(layout);

			// Accumulate all the memory requirements of all the sets
			for (auto& entry : layout.get().required_pool_sizes()) {
				auto it = std::lower_bound(std::begin(result.mAccumulatedSizes), std::end(result.mAccumulatedSizes), 
					entry,
					[](const vk::DescriptorPoolSize& first, const vk::DescriptorPoolSize& second) -> bool {
						using EnumType = std::underlying_type<vk::DescriptorType>::type;
						return static_cast<EnumType>(first.type) < static_cast<EnumType>(second.type);
					});
				if (it != std::end(result.mAccumulatedSizes) && it->type == entry.type) {
					it->descriptorCount += entry.descriptorCount;
				}
				else {
					result.mAccumulatedSizes.insert(it, entry);
				}
			}
		}

		return result;
	}



	std::shared_ptr<descriptor_pool> descriptor_pool::create(const std::vector<vk::DescriptorPoolSize>& pSizeRequirements, uint32_t numSets)
	{
		descriptor_pool result;
		result.mCapacities = pSizeRequirements;
		result.mNumInitialSets = numSets;
		result.mNumRemainingSets = numSets;

		// Create it:
		auto createInfo = vk::DescriptorPoolCreateInfo()
			.setPoolSizeCount(static_cast<uint32_t>(pSizeRequirements.size()))
			.setPPoolSizes(pSizeRequirements.data())
			.setMaxSets(numSets) 
			.setFlags(vk::DescriptorPoolCreateFlags()); // The structure has an optional flag similar to command pools that determines if individual descriptor sets can be freed or not: VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT. We're not going to touch the descriptor set after creating it, so we don't need this flag. [10]
		result.mDescriptorPool = context().logical_device().createDescriptorPoolUnique(createInfo);

		return std::make_shared<descriptor_pool>(std::move(result));
	}

	bool descriptor_pool::has_capacity_for(const descriptor_alloc_request& pRequest)
	{
		if (mNumRemainingSets == 0) {
			return false;
		}

		const auto& weNeed = pRequest.accumulated_pool_sizes();
		const auto& weHave = mCapacities;

		// Accumulate all the requirements of all the sets
		using EnumType = std::underlying_type<vk::DescriptorType>::type;
		size_t n = 0, h = 0, N = weNeed.size(), H = weHave.size();
		while (n < N && h < H) {
			auto needType = static_cast<EnumType>(weNeed[n].type);
			auto haveType = static_cast<EnumType>(weHave[h].type);
			if (haveType < needType) {
				h++; 
				continue;
			}
			if (needType < haveType) {
				return false;
			}
			// same type:
			if (weNeed[n].descriptorCount <= weHave[n].descriptorCount) {
				n++;
				continue;
			}
		}
		// all checks passed
		return true;
	}

	std::vector<descriptor_set> descriptor_pool::allocate(const descriptor_alloc_request& pRequest)
	{
		std::vector<descriptor_set> result;
		return result;
	}
}
