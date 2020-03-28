#include <cg_base.hpp>

namespace cgb
{
	descriptor_set_layout descriptor_set_layout::prepare(std::vector<binding_data> pBindings)
	{
		return prepare(std::begin(pBindings), std::end(pBindings));
	}

	void descriptor_set_layout::allocate()
	{
		if (!mLayout) {
			// Allocate the layout and return the result:
			auto createInfo = vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(static_cast<uint32_t>(mOrderedBindings.size()))
				.setPBindings(mOrderedBindings.data());
			mLayout = cgb::context().logical_device().createDescriptorSetLayoutUnique(createInfo);
		}
		else {
			LOG_ERROR("descriptor_set_layout's handle already has a value => it most likely has already been allocated. Won't do it again.");
		}
	}


	set_of_descriptor_set_layouts set_of_descriptor_set_layouts::prepare(std::vector<binding_data> pBindings)
	{
		set_of_descriptor_set_layouts result;
		std::vector<binding_data> orderedBindings;
		uint32_t minSetId = std::numeric_limits<uint32_t>::max();
		uint32_t maxSetId = std::numeric_limits<uint32_t>::min();

		// Step 1: order the bindings
		for (auto& b : pBindings) {
			minSetId = std::min(minSetId, b.mSetId);
			maxSetId = std::max(maxSetId, b.mSetId);
			auto it = std::lower_bound(std::begin(orderedBindings), std::end(orderedBindings), b); // use operator<
			orderedBindings.insert(it, b);
		}

		// Step 2: assemble the separate sets
		result.mFirstSetId = minSetId;
		result.mLayouts.reserve(maxSetId - minSetId + 1);
		for (uint32_t setId = minSetId; setId <= maxSetId; ++setId) {
			auto lb = std::lower_bound(std::begin(orderedBindings), std::end(orderedBindings), binding_data{ setId },
				[](const binding_data& first, const binding_data& second) -> bool {
					return first.mSetId < second.mSetId;
				});
			auto ub = std::upper_bound(std::begin(orderedBindings), std::end(orderedBindings), binding_data{ setId },
				[](const binding_data& first, const binding_data& second) -> bool {
					return first.mSetId < second.mSetId;
				});
			result.mLayouts.push_back(descriptor_set_layout::prepare(lb, ub));
		}

		// Step 3: Accumulate the binding requirements a.k.a. vk::DescriptorPoolSize entries
		for (auto& dsl : result.mLayouts) {
			for (auto& dps : dsl.required_pool_sizes()) {
				// find position where to insert in vector
				auto it = std::lower_bound(std::begin(result.mBindingRequirements), std::end(result.mBindingRequirements),
					dps,
					[](const vk::DescriptorPoolSize& first, const vk::DescriptorPoolSize& second) -> bool {
						using EnumType = std::underlying_type<vk::DescriptorType>::type;
						return static_cast<EnumType>(first.type) < static_cast<EnumType>(second.type);
					});
				// Maybe accumulate
				if (it != std::end(result.mBindingRequirements) && it->type == dps.type) {
					it->descriptorCount += dps.descriptorCount;
				}
				else {
					result.mBindingRequirements.insert(it, dps);
				}
			}
		}

		// Done with the preparation. (None of the descriptor_set_layout members have been allocated at this point.)
		return result;
	}

	void set_of_descriptor_set_layouts::allocate_all()
	{
		for (auto& dsl : mLayouts) {
			dsl.allocate();
		}
	}

	std::vector<vk::DescriptorSetLayout> set_of_descriptor_set_layouts::layout_handles() const
	{
		std::vector<vk::DescriptorSetLayout> allHandles;
		allHandles.reserve(mLayouts.size());
		for (auto& dsl : mLayouts) {
			allHandles.push_back(dsl.handle());
		}
		return allHandles;
	}

	void descriptor_set::link_to_handle_and_pool(vk::UniqueDescriptorSet aHandle, std::shared_ptr<descriptor_pool> aPool)
	{
		mDescriptorSet = std::move(aHandle);
		for (auto& w : mOrderedDescriptorDataWrites) {
			w.setDstSet(mDescriptorSet.get());
		}
		mPool = std::move(aPool);
	}
	
	void descriptor_set::write_descriptors()
	{
		assert(mDescriptorSet);
		cgb::context().logical_device().updateDescriptorSets(static_cast<uint32_t>(mOrderedDescriptorDataWrites.size()), mOrderedDescriptorDataWrites.data(), 0u, nullptr);
	}
	
	std::vector<const descriptor_set*> descriptor_set::create(std::initializer_list<binding_data> aBindings, descriptor_cache_interface* aCache)
	{
		std::vector<binding_data> orderedBindings;
		uint32_t minSetId = std::numeric_limits<uint32_t>::max();
		uint32_t maxSetId = std::numeric_limits<uint32_t>::min();

		// Step 1: order the bindings
		for (auto& b : aBindings) {
			minSetId = std::min(minSetId, b.mSetId);
			maxSetId = std::max(maxSetId, b.mSetId);
			auto it = std::lower_bound(std::begin(orderedBindings), std::end(orderedBindings), b); // use operator<
			orderedBindings.insert(it, b);
		}

		std::vector<std::reference_wrapper<const descriptor_set_layout>> layouts;
		std::vector<descriptor_set> preparedSets;
		std::vector<const descriptor_set*> cachedSets;
		int numCached = 0;
		// Step 2: go through all the sets, get or alloc layouts, and see if the descriptor sets are already in cache, by chance.
		for (uint32_t setId = minSetId; setId <= maxSetId; ++setId) {
			auto lb = std::lower_bound(std::begin(orderedBindings), std::end(orderedBindings), binding_data{ setId },
				[](const binding_data& first, const binding_data& second) -> bool {
					return first.mSetId < second.mSetId;
				});
			auto ub = std::upper_bound(std::begin(orderedBindings), std::end(orderedBindings), binding_data{ setId },
				[](const binding_data& first, const binding_data& second) -> bool {
					return first.mSetId < second.mSetId;
				});

			const auto& layout = aCache->get_or_alloc_layout(descriptor_set_layout::prepare(lb, ub));
			auto preparedSet = descriptor_set::prepare(lb, ub);
			const auto* cachedSet = aCache->get_descriptor_set_from_cache(preparedSet);
			layouts.emplace_back(cachedSet);
			numCached += nullptr != cachedSet ? 1 : 0;
			preparedSets.emplace_back(std::move(preparedSet));
		}

		// Everything is cached; we're done.
		if (cachedSets.size() == numCached) {
			return cachedSets;
		}

		// HOWEVER, if not...
		return aCache->alloc_descriptor_sets(layouts, std::move(preparedSets));


		// Old, deprecated code:
		//
		// TODO: Delete when done with DescriptorSets new!
		//
		//
		//
		//auto setOfLayouts = set_of_descriptor_set_layouts::prepare(aBindings);
		//auto nSets = setOfLayouts.number_of_sets();
		//for (decltype(nSets) i = 0; i < nSets; ++i) {

		//	
		//	
		//}

		//result.mDescriptorSetOwners = cgb::context().create_descriptor_set(result.mSetOfLayouts.layout_handles());
		//result.mDescriptorSets.reserve(result.mDescriptorSetOwners.size());
		//for (auto& uniqueDesc : result.mDescriptorSetOwners) { // TODO: Is the second array really neccessary?
		//	result.mDescriptorSets.push_back(uniqueDesc.get());
		//}


		//std::vector<vk::WriteDescriptorSet> descriptorWrites;
		//for (auto& b : pBindings) {
		//	descriptorWrites.push_back(vk::WriteDescriptorSet{}
		//		// descriptor sets are perfectly aligned with layouts (I hope so, at least)
		//		.setDstSet(result.mDescriptorSets[result.mSetOfLayouts.set_index_for_set_id(b.mSetId)])
		//		.setDstBinding(b.mLayoutBinding.binding)
		//		.setDstArrayElement(0u) // TODO: support more
		//		.setDescriptorType(b.mLayoutBinding.descriptorType) // TODO: Okay or use that one stored in the mResourcePtr??
		//		.setDescriptorCount(b.descriptor_count()) // TODO: Okay?
		//		.setPBufferInfo(b.descriptor_buffer_info())
		//		.setPImageInfo(b.descriptor_image_info())
		//		.setPTexelBufferView(b.texel_buffer_view_info())
		//		.setPNext(b.next_pointer())
		//	);
		//	// TODO: Handle array types!
		//}

		//return result;
		//// Hmm... kann das funktionieren? k.A. Es wäre einmal eine (eigentlich vollständige) Implementierung (denke ich)
	}
}
