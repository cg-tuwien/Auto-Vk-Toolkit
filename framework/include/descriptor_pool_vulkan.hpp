#pragma once

namespace cgb
{
	/** Data about an allocation which is to be passed to a descriptor pool 
	 *	Actually this is only a helper class. The passed descriptor_set_layout references
	 *	must outlive an instance of this class under all circumstances!!
	 */
	class descriptor_alloc_request
	{
	public:
		descriptor_alloc_request() = default;
		descriptor_alloc_request(descriptor_alloc_request&&) = default;
		descriptor_alloc_request& operator=(const descriptor_alloc_request&) = delete;
		descriptor_alloc_request& operator=(descriptor_alloc_request&&) = default;
		~descriptor_alloc_request() = default;

		auto& layouts_to_allocate() const { return mToAlloc; }
		auto& accumulated_pool_sizes() const { return mAccumulatedSizes; }

		static descriptor_alloc_request create(std::initializer_list<std::reference_wrapper<descriptor_set_layout>> pLayouts);

	private:
		std::vector<std::reference_wrapper<descriptor_set_layout>> mToAlloc;
		std::vector<vk::DescriptorPoolSize> mAccumulatedSizes;
	};

	/** A descriptor pool which can allocate storage for descriptor sets from descriptor layouts.
	 */
	class descriptor_pool
	{
	public:
		descriptor_pool() = default;
		descriptor_pool(descriptor_pool&&) = default;
		descriptor_pool& operator=(const descriptor_pool&) = delete;
		descriptor_pool& operator=(descriptor_pool&&) = default;
		~descriptor_pool() = default;

		const auto& handle() const { return mDescriptorPool.get(); }

		bool has_capacity_for(const descriptor_alloc_request& pRequest);
		std::vector<descriptor_set> allocate(const descriptor_alloc_request& pRequest);
		
		static std::shared_ptr<descriptor_pool> create(const std::vector<vk::DescriptorPoolSize>& pSizeRequirements, uint32_t numSets);
		
	private:
		vk::UniqueDescriptorPool mDescriptorPool;
		std::vector<vk::DescriptorPoolSize> mCapacities;
		uint32_t mNumInitialSets;
		uint32_t mNumRemainingSets;
	};
}
