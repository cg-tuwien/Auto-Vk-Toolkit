#pragma once

namespace cgb
{
	/** TBD
	*/
	template <typename Meta>
	class buffer_t
	{
		template <typename T>
		friend cgb::owning_resource<buffer_t<T>> create(T pConfig);

	public:
		buffer_t() = default;
		buffer_t(const buffer_t&) = delete;
		buffer_t(buffer_t&&) noexcept = default;
		buffer_t& operator=(const buffer_t&) = delete;
		buffer_t& operator=(buffer_t&&) noexcept = default;
		~buffer_t() = default; // Declaration order determines destruction order (inverse!)

	public:
		Meta mMetaData;
		context_tracker<buffer_t<Meta>> mTracker;
	};


	/**	TBD
	*/
	template <typename Meta>
	cgb::owning_resource<buffer_t<Meta>> create(Meta pConfig)
	{
		cgb::buffer_t<Meta> b;
		b.mMetaData = pConfig;
		b.mTracker.setTrackee(b);
		return std::move(b);
	}

	/**	TBD
	*/
	template <typename Meta>
	void fill(
		const cgb::buffer_t<Meta>& target,
		const void* pData,
		std::function<void(owning_resource<semaphore_t>)> pSemaphoreHandler = nullptr)
	{
		
	}

	// For convenience:

	inline void fill(const generic_buffer_t& target, const void* pData, std::function<void(owning_resource<semaphore_t>)> pSemaphoreHandler = nullptr)			{ fill<generic_buffer_meta>(target, pData, std::move(pSemaphoreHandler)); }
	inline void fill(const uniform_buffer_t& target, const void* pData, std::function<void(owning_resource<semaphore_t>)> pSemaphoreHandler = nullptr)			{ fill<uniform_buffer_meta>(target, pData, std::move(pSemaphoreHandler)); }
	inline void fill(const uniform_texel_buffer_t& target, const void* pData, std::function<void(owning_resource<semaphore_t>)> pSemaphoreHandler = nullptr)	{ fill<uniform_texel_buffer_meta>(target, pData, std::move(pSemaphoreHandler)); }
	inline void fill(const storage_buffer_t& target, const void* pData, std::function<void(owning_resource<semaphore_t>)> pSemaphoreHandler = nullptr)			{ fill<storage_buffer_meta>(target, pData, std::move(pSemaphoreHandler)); }
	inline void fill(const storage_texel_buffer_t& target, const void* pData, std::function<void(owning_resource<semaphore_t>)> pSemaphoreHandler = nullptr)	{ fill<storage_texel_buffer_meta>(target, pData, std::move(pSemaphoreHandler)); }
	inline void fill(const vertex_buffer_t& target, const void* pData, std::function<void(owning_resource<semaphore_t>)> pSemaphoreHandler = nullptr)			{ fill<vertex_buffer_meta>(target, pData, std::move(pSemaphoreHandler)); }
	inline void fill(const index_buffer_t& target, const void* pData, std::function<void(owning_resource<semaphore_t>)> pSemaphoreHandler = nullptr)			{ fill<index_buffer_meta>(target, pData, std::move(pSemaphoreHandler)); }
	inline void fill(const instance_buffer_t& target, const void* pData, std::function<void(owning_resource<semaphore_t>)> pSemaphoreHandler = nullptr)			{ fill<instance_buffer_meta>(target, pData, std::move(pSemaphoreHandler)); }

	// CREATE AND FILL
	template <typename Meta>
	cgb::owning_resource<buffer_t<Meta>> create_and_fill(
		Meta pConfig, 
		cgb::memory_usage pMemoryUsage, 
		const void* pData, 
		std::function<void(owning_resource<semaphore_t>)> pSemaphoreHandler = nullptr)
	{
		cgb::owning_resource<buffer_t<Meta>> result = create(pConfig);
		fill(static_cast<const cgb::buffer_t<Meta>&>(result), pData, std::move(pSemaphoreHandler));
		return std::move(result);
	}
}
