#include <cg_base.hpp>

namespace cgb
{
	descriptor_alloc_request descriptor_alloc_request::create(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts)
	{
		descriptor_alloc_request result;
		result.mNumSets = static_cast<uint32_t>(aLayouts.size());

		for (const auto& layout : aLayouts) {
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



	std::shared_ptr<descriptor_pool> descriptor_pool::create(const std::vector<vk::DescriptorPoolSize>& pSizeRequirements, int numSets)
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

	bool descriptor_pool::has_capacity_for(const descriptor_alloc_request& pRequest) const
	{
		if (mNumRemainingSets < static_cast<int>(pRequest.num_sets())) {
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

	std::vector<vk::UniqueDescriptorSet> descriptor_pool::allocate(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts)
	{
		std::vector<vk::DescriptorSetLayout> setLayouts;
		setLayouts.reserve(aLayouts.size());
		for (auto& in : aLayouts) {
			setLayouts.emplace_back(in.get().handle());
		}
		
		auto allocInfo = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(mDescriptorPool.get()) 
			.setDescriptorSetCount(static_cast<uint32_t>(setLayouts.size()))
			.setPSetLayouts(setLayouts.data());

		// Update the pool's stats:
		mNumRemainingSets -= static_cast<int>(aLayouts.size());

		return cgb::context().logical_device().allocateDescriptorSetsUnique(allocInfo);
	}
}
