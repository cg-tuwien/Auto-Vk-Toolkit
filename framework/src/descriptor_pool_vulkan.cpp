#include <cg_base.hpp>

namespace cgb
{
	void descriptor_alloc_request::add_size_requirements(vk::DescriptorPoolSize aToAdd)
	{
		auto it = std::lower_bound(std::begin(mAccumulatedSizes), std::end(mAccumulatedSizes), 
			aToAdd,
			[](const vk::DescriptorPoolSize& first, const vk::DescriptorPoolSize& second) -> bool {
				using EnumType = std::underlying_type<vk::DescriptorType>::type;
				return static_cast<EnumType>(first.type) < static_cast<EnumType>(second.type);
			});
		if (it != std::end(mAccumulatedSizes) && it->type == aToAdd.type) {
			it->descriptorCount += aToAdd.descriptorCount;
		}
		else {
			mAccumulatedSizes.insert(it, aToAdd);
		}
	}

	descriptor_alloc_request descriptor_alloc_request::multiply_size_requirements(uint32_t mFactor) const
	{
		auto copy = descriptor_alloc_request{*this};
		for (auto& sr : copy.mAccumulatedSizes) {
			sr.descriptorCount *= mFactor;
		}
		return copy;
	}
	
	descriptor_alloc_request descriptor_alloc_request::create(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts)
	{
		descriptor_alloc_request result;
		result.mNumSets = static_cast<uint32_t>(aLayouts.size());

		for (const auto& layout : aLayouts) {
			// Accumulate all the memory requirements of all the sets
			for (const auto& entry : layout.get().required_pool_sizes()) {
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
	
	std::shared_ptr<descriptor_pool> descriptor_pool::create(const std::vector<vk::DescriptorPoolSize>& aSizeRequirements, int aNumSets)
	{
		descriptor_pool result;
		result.mInitialCapacities = aSizeRequirements;
		result.mRemainingCapacities = aSizeRequirements;
		result.mNumInitialSets = aNumSets;
		result.mNumRemainingSets = aNumSets;

		// Create it:
		auto createInfo = vk::DescriptorPoolCreateInfo()
			.setPoolSizeCount(static_cast<uint32_t>(result.mInitialCapacities.size()))
			.setPPoolSizes(result.mInitialCapacities.data())
			.setMaxSets(aNumSets) 
			.setFlags(vk::DescriptorPoolCreateFlags()); // The structure has an optional flag similar to command pools that determines if individual descriptor sets can be freed or not: VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT. We're not going to touch the descriptor set after creating it, so we don't need this flag. [10]
		result.mDescriptorPool = context().logical_device().createDescriptorPoolUnique(createInfo);
		
		LOG_DEBUG_VERBOSE(fmt::format("Allocated pool[{}] with flags[{}], maxSets[{}], remaining-sets[{}], size-entries[{}]", result.mDescriptorPool.get(), to_string(createInfo.flags), createInfo.maxSets, result.mNumRemainingSets, createInfo.poolSizeCount));
#if defined(_DEBUG) && LOG_LEVEL > 3
		for (int i=0; i < aSizeRequirements.size(); ++i) {
			LOG_DEBUG_VERBOSE(fmt::format("          [{}]: descriptorCount[{}], descriptorType[{}]", i, aSizeRequirements[i].descriptorCount, to_string(aSizeRequirements[i].type)));
		}
#endif
		
		return std::make_shared<descriptor_pool>(std::move(result));
	}

	bool descriptor_pool::has_capacity_for(const descriptor_alloc_request& pRequest) const
	{
		//if (mNumRemainingSets < static_cast<int>(pRequest.num_sets())) {
		//	return false;
		//}

		const auto& weNeed = pRequest.accumulated_pool_sizes();
		const auto& weHave = mRemainingCapacities;

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
			if (needType == haveType && weNeed[n].descriptorCount <= weHave[n].descriptorCount) {
				n++;
				h++;
				continue;
			}
			return false;
		}
		return n == h; // if true => all checks have passed, if false => checks failed
	}

	std::vector<vk::DescriptorSet> descriptor_pool::allocate(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts)
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

		LOG_DEBUG_VERBOSE(fmt::format("About to allocate from pool[{}] with remaining-sets[{}] and remaining-capacities:", mDescriptorPool.get(), mNumRemainingSets));
#if defined(_DEBUG) && LOG_LEVEL > 3
		for (int i=0; i < mRemainingCapacities.size(); ++i) {
			LOG_DEBUG_VERBOSE(fmt::format("          [{}]: descriptorCount[{}], descriptorType[{}]", i, mRemainingCapacities[i].descriptorCount, to_string(mRemainingCapacities[i].type)));
		}
#endif
		LOG_DEBUG_VERBOSE(fmt::format("...going to allocate {} set{}of the following:", aLayouts.size(), aLayouts.size() > 1 ? "s " : " "));
#if defined(_DEBUG) && LOG_LEVEL > 3
		for (int i=0; i < aLayouts.size(); ++i) {
			LOG_DEBUG_VERBOSE(fmt::format("          [{}]: number_of_bindings[{}]", i, aLayouts[i].get().number_of_bindings()));
			for (int j=0; j < aLayouts[i].get().number_of_bindings(); j++) {
				LOG_DEBUG_VERBOSE(fmt::format("               [{}]: descriptorCount[{}], descriptorType[{}]", j, aLayouts[i].get().binding_at(j).descriptorCount, to_string(aLayouts[i].get().binding_at(j).descriptorType)));
			}
			LOG_DEBUG_VERBOSE(fmt::format("          [{}]: required pool sizes (whatever the difference to 'bindings' is)", i));
			auto& rps = aLayouts[i].get().required_pool_sizes();
			for (int j=0; j < rps.size(); j++) {
				LOG_DEBUG_VERBOSE(fmt::format("               [{}]: descriptorCount[{}], descriptorType[{}]", j, rps[j].descriptorCount, to_string(rps[j].type)));
			}
		}
#endif

		auto result = cgb::context().logical_device().allocateDescriptorSets(allocInfo);

		// Update the pool's stats:
		for (const descriptor_set_layout& dsl : aLayouts) {
			for (const auto& dps : dsl.required_pool_sizes()) {
				auto it = std::find_if(std::begin(mRemainingCapacities), std::end(mRemainingCapacities), [&dps](vk::DescriptorPoolSize& el){
					return el.type == dps.type;
				});
				if (std::end(mRemainingCapacities) == it) {
					LOG_WARNING("Couldn't find the descriptor type that we have just allocated in mRemainingCapacities. How could this have happened?");
				}
				else {
					it->descriptorCount -= std::min(dps.descriptorCount, it->descriptorCount);
				}
			}
		}

		mNumRemainingSets -= static_cast<int>(aLayouts.size());

		return result;
	}
}
