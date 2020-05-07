#pragma once

namespace cgb
{
	/** Data about an allocation which is to be passed to a descriptor pool 
	 *	Actually this is only a helper class.
	 */
	class descriptor_alloc_request
	{
	public:
		descriptor_alloc_request() = default;
		descriptor_alloc_request(descriptor_alloc_request&&) noexcept = default;
		descriptor_alloc_request(const descriptor_alloc_request&) = default;
		descriptor_alloc_request& operator=(descriptor_alloc_request&&) noexcept = default;
		descriptor_alloc_request& operator=(const descriptor_alloc_request&) = default;
		~descriptor_alloc_request() = default;

		void add_size_requirements(vk::DescriptorPoolSize aToAdd);
		const auto& accumulated_pool_sizes() const { return mAccumulatedSizes; }
		void set_num_sets(uint32_t aNumSets) { mNumSets = aNumSets; }
		auto num_sets() const { return mNumSets; }

		descriptor_alloc_request multiply_size_requirements(uint32_t mFactor) const;

		/**	Gather information about the required alloc size of given layouts
		 */
		static descriptor_alloc_request create(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts);
		
	private:
		std::vector<vk::DescriptorPoolSize> mAccumulatedSizes;
		uint32_t mNumSets;
	};
	
	/** A descriptor pool which can allocate storage for descriptor sets from descriptor layouts.
	 */
	class descriptor_pool
	{
	public:
		descriptor_pool() = default;
		descriptor_pool(descriptor_pool&&) noexcept = default;
		descriptor_pool& operator=(const descriptor_pool&) = delete;
		descriptor_pool& operator=(descriptor_pool&&) noexcept = default;
		~descriptor_pool() = default;

		const auto& handle() const { return mDescriptorPool.get(); }

		bool has_capacity_for(const descriptor_alloc_request& pRequest) const;
		std::vector<vk::DescriptorSet> allocate(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts);
		// TODO: IF required, implement a std::vector<vk::DescriptorSet> allocate_unique method!
		
		static std::shared_ptr<descriptor_pool> create(const std::vector<vk::DescriptorPoolSize>& pSizeRequirements, int numSets);
		
	private:
		vk::UniqueDescriptorPool mDescriptorPool;
		std::vector<vk::DescriptorPoolSize> mCapacities;
		int mNumInitialSets;
		int mNumRemainingSets;
	};
}
