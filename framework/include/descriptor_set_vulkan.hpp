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
	public:
		descriptor_set_layout() = default;
		descriptor_set_layout(const descriptor_set_layout&) = delete;
		descriptor_set_layout(descriptor_set_layout&&) = default;
		descriptor_set_layout& operator=(const descriptor_set_layout&) = delete;
		descriptor_set_layout& operator=(descriptor_set_layout&&) = default;
		~descriptor_set_layout() = default;

		const auto& required_pool_sizes() const { return mBindingRequirements; }
		const auto& bindings() const { return mOrderedBindings; }
		auto handle() const { return mLayout.get(); }

		template <typename It>
		static descriptor_set_layout prepare(It begin, It end)
		{
			// Put elements from initializer list into vector of ORDERED bindings:
			descriptor_set_layout result;
			
#if defined(_DEBUG)
			std::optional<uint32_t> setId;
#endif

			It it = begin;
			while (it != end) {
				const binding_data& b = *it;

#if defined(_DEBUG)
				if (!setId.has_value()) {
					setId = b.mSetId;
				} 
				else {
					assert(setId == b.mSetId);
				}
#endif

				{ // Compile the mBindingRequirements member:
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

				{ // Compile the mOrderedBindings member:
				  // ordered by binding => find position where to insert in vector
					auto pos = std::lower_bound(std::begin(result.mOrderedBindings), std::end(result.mOrderedBindings), 
						b.mLayoutBinding,
						[](const vk::DescriptorSetLayoutBinding& first, const vk::DescriptorSetLayoutBinding& second) -> bool {
							assert(first.binding != second.binding);
							return first.binding < second.binding;
						});
					result.mOrderedBindings.insert(pos, b.mLayoutBinding);
				}

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

	/** Basically a vector of descriptor_set_layout instances */
	class set_of_descriptor_set_layouts
	{
	public:
		set_of_descriptor_set_layouts() = default;
		set_of_descriptor_set_layouts(const set_of_descriptor_set_layouts&) = delete;
		set_of_descriptor_set_layouts(set_of_descriptor_set_layouts&&) = default;
		set_of_descriptor_set_layouts& operator=(const set_of_descriptor_set_layouts&) = delete;
		set_of_descriptor_set_layouts& operator=(set_of_descriptor_set_layouts&&) = default;
		~set_of_descriptor_set_layouts() = default;

		static set_of_descriptor_set_layouts prepare(std::vector<binding_data> pBindings);

		void allocate_all();

		uint32_t number_of_sets() const { return static_cast<uint32_t>(mLayouts.size()); }
		const auto& set_at(uint32_t pIndex) const { return mLayouts[pIndex]; }
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
	public:
		descriptor_set() = default;
		descriptor_set(const descriptor_set&) = delete;
		descriptor_set(descriptor_set&&) = default;
		descriptor_set& operator=(const descriptor_set&) = delete;
		descriptor_set& operator=(descriptor_set&&) = default;
		~descriptor_set() = default;

		static descriptor_set create(std::initializer_list<binding_data> pBindings);

	private:
		std::vector<vk::DescriptorSet> mDescriptorSets;
	};
}
