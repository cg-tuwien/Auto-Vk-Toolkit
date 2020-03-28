#include <cg_base.hpp>

namespace cgb
{
	const descriptor_set_layout& standard_descriptor_cache::get_or_alloc_layout(descriptor_set_layout aPreparedLayout)
	{
		const auto it = mLayouts.find(aPreparedLayout);
		if (mLayouts.end() != it) {
			assert(it->handle());
			return *it;
		}
		aPreparedLayout.allocate();
		const auto result = mLayouts.insert(std::move(aPreparedLayout));
		assert(result.second);
		return *result.first;
	}

	const descriptor_set* standard_descriptor_cache::get_descriptor_set_from_cache(const descriptor_set& aPreparedSet)
	{
		const auto it = mSets.find(aPreparedSet);
		if (mSets.end() != it) {
			return &(*it);
		}
		return nullptr;
	}

	std::vector<const descriptor_set*> standard_descriptor_cache::alloc_descriptor_sets(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts, std::vector<descriptor_set> aPreparedSets)
	{
		assert(aLayouts.size() == aPreparedSets.size());

		std::vector<const descriptor_set*> result;
		const auto n = aLayouts.size();
#ifdef _DEBUG // Perform an extensive sanity check:
		for (size_t i = 0; i < n; ++i) {
			const auto dbgB = aLayouts[i].get().number_of_bindings();
			assert(dbgB == aPreparedSets[i].number_of_writes());
			for (size_t j = 0; j < dbgB; ++j) {
				assert(aLayouts[i].get().binding_at(j).binding			== aPreparedSets[i].write_at(j).dstBinding);
				assert(aLayouts[i].get().binding_at(j).descriptorCount	== aPreparedSets[i].write_at(j).descriptorCount);
				assert(aLayouts[i].get().binding_at(j).descriptorType	== aPreparedSets[i].write_at(j).descriptorType);
			}
		}
#endif
		
		const auto allocRequest = descriptor_alloc_request::create(aLayouts);
		auto pool = cgb::context().get_descriptor_pool_for_layouts(allocRequest, 'stch');
		assert(pool->has_capacity_for(allocRequest));
		// Alloc the whole thing:
		auto setHandles = pool->allocate(aLayouts);
		assert(setHandles.size() == aPreparedSets.size());
		for (size_t i = 0; i < n; ++i) {
			auto& setToBeCompleted = aPreparedSets[i];
			setToBeCompleted.link_to_handle_and_pool(std::move(setHandles[i]), pool);
			setToBeCompleted.write_descriptors();
			// Your soul... is mine:
			const auto cachedSet = mSets.insert(std::move(setToBeCompleted));
			assert(cachedSet.second);
			// Done. Store for result:
			result.push_back(&(*cachedSet.first));
		}

		return result;
	}
}
