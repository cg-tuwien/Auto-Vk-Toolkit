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
		descriptor_alloc_request& operator=(const descriptor_alloc_request&) = delete;
		descriptor_alloc_request& operator=(descriptor_alloc_request&&) noexcept = default;
		~descriptor_alloc_request() = default;

		auto& accumulated_pool_sizes() const { return mAccumulatedSizes; }

		static descriptor_alloc_request create(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts);

	private:
		std::vector<vk::DescriptorPoolSize> mAccumulatedSizes;
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
		std::vector<vk::UniqueDescriptorSet> allocate(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts);
		
		static std::shared_ptr<descriptor_pool> create(const std::vector<vk::DescriptorPoolSize>& pSizeRequirements, uint32_t numSets);
		
	private:
		vk::UniqueDescriptorPool mDescriptorPool;
		std::vector<vk::DescriptorPoolSize> mCapacities;
		uint32_t mNumInitialSets;
		uint32_t mNumRemainingSets;
	};
}
