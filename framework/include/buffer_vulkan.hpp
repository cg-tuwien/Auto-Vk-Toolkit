#pragma once

namespace cgb
{
	/** Represents a Vulkan buffer along with its assigned memory, holds the 
	*	native handle and takes care about lifetime management of the native handles.
	*/
	template <typename Meta>
	class buffer_t
	{
		template <typename T>
		friend cgb::owning_resource<buffer_t<T>> create(T pConfig, vk::BufferUsageFlags pBufferUsage, vk::MemoryPropertyFlags pMemoryProperties, std::optional<vk::DescriptorType> pDescriptorType);

	public:
		buffer_t() = default;
		buffer_t(buffer_t&&) noexcept = default;
		buffer_t(const buffer_t&) = delete;
		buffer_t& operator=(buffer_t&&) noexcept = default;
		buffer_t& operator=(const buffer_t&) = delete;
		~buffer_t() = default; // Declaration order determines destruction order (inverse!)

		const auto& meta_data() const			{ return mMetaData; }
		auto size() const						{ return mMetaData.total_size(); }
		auto& config() const					{ return mCreateInfo; }
		const auto& memory_properties() const	{ return mMemoryPropertyFlags; }
		const auto& memory_handle() const		{ return mMemory.get(); }
		const auto* memory_handle_addr() const	{ return &mMemory.get(); }
		const auto& buffer_usage_flags() const	{ return mBufferUsageFlags; }
		const auto& buffer_handle() const		{ return mBuffer.get(); }
		const auto* buffer_handle_addr() const	{ return &mBuffer.get(); }
		const auto& descriptor_info() const		{ return mDescriptorInfo; }
		auto has_descriptor_type() const		{ return mDescriptorType.has_value(); }
		auto descriptor_type() const			{ return mDescriptorType.value(); } //< might throw

	public:
		Meta mMetaData;
		vk::BufferCreateInfo mCreateInfo;
		vk::MemoryPropertyFlags mMemoryPropertyFlags;
		vk::UniqueDeviceMemory mMemory;
		vk::BufferUsageFlags mBufferUsageFlags;
		vk::UniqueBuffer mBuffer;
		vk::DescriptorBufferInfo mDescriptorInfo;
		std::optional<vk::DescriptorType> mDescriptorType;
		context_tracker<buffer_t<Meta>> mTracker;
	};


	/**	Create a buffer which is always created with exclusive access for a queue.
	*	If different queues are being used, ownership has to be transferred explicitely.
	*/
	template <typename Meta>
	cgb::owning_resource<buffer_t<Meta>> create(Meta pConfig, vk::BufferUsageFlags pBufferUsage, vk::MemoryPropertyFlags pMemoryProperties, std::optional<vk::DescriptorType> pDescriptorType = {})
	{
		auto bufferSize = pConfig.total_size();

		// Create (possibly multiple) buffer(s):
		auto bufferCreateInfo = vk::BufferCreateInfo()
			.setSize(static_cast<vk::DeviceSize>(bufferSize))
			.setUsage(pBufferUsage)
			// Always grant exclusive ownership to the queue.
			.setSharingMode(vk::SharingMode::eExclusive)
			// The flags parameter is used to configure sparse buffer memory, which is not relevant right now. We'll leave it at the default value of 0. [2]
			.setFlags(vk::BufferCreateFlags()); 

		// Create the buffer on the logical device
		auto vkBuffer = cgb::context().logical_device().createBufferUnique(bufferCreateInfo);

		// The buffer has been created, but it doesn't actually have any memory assigned to it yet. 
		// The first step of allocating memory for the buffer is to query its memory requirements [2]
		auto memRequirements = cgb::context().logical_device().getBufferMemoryRequirements(vkBuffer.get());

		auto allocInfo = vk::MemoryAllocateInfo()
			.setAllocationSize(memRequirements.size)
			.setMemoryTypeIndex(cgb::context().find_memory_type_index(
				memRequirements.memoryTypeBits, 
				pMemoryProperties));

		// Allocate the memory for the buffer:
		auto vkMemory = cgb::context().logical_device().allocateMemoryUnique(allocInfo);

		// If memory allocation was successful, then we can now associate this memory with the buffer
		cgb::context().logical_device().bindBufferMemory(vkBuffer.get(), vkMemory.get(), 0);
		// TODO: if(!succeeded) { throw std::runtime_error("Binding memory to buffer failed."); }

		cgb::buffer_t<Meta> b;
		b.mMetaData = pConfig;
		b.mCreateInfo = bufferCreateInfo;
		b.mMemoryPropertyFlags = pMemoryProperties;
		b.mMemory = std::move(vkMemory);
		b.mBufferUsageFlags = pBufferUsage;
		b.mBuffer = std::move(vkBuffer);
		b.mDescriptorInfo = vk::DescriptorBufferInfo()
			.setBuffer(b.buffer_handle())
			.setOffset(0)
			.setRange(b.size());
		b.mDescriptorType = pDescriptorType;
		b.mTracker.setTrackee(b);
		return std::move(b);
	}

	// Forward-declare what comes after:
	template <typename Meta>
	cgb::owning_resource<buffer_t<Meta>> create_and_fill(
		Meta pConfig, 
		cgb::memory_usage pMemoryUsage, 
		const void* pData, 
		sync aSyncHandler,
		vk::BufferUsageFlags pUsage);

	// SET DATA
	template <typename Meta>
	void fill(
		const cgb::buffer_t<Meta>& target,
		const void* pData, 
		sync aSyncHandler = sync::wait_idle())
	{
		auto bufferSize = static_cast<vk::DeviceSize>(target.size());

		auto memProps = target.memory_properties();

		// #1: Is our memory on the CPU-SIDE? 
		if (cgb::has_flag(memProps, vk::MemoryPropertyFlagBits::eHostVisible)) {
			void* mapped = cgb::context().logical_device().mapMemory(target.memory_handle(), 0, bufferSize);
			memcpy(mapped, pData, bufferSize);
			// Coherent memory is done; non-coherent memory not yet
			if (!cgb::has_flag(memProps, vk::MemoryPropertyFlagBits::eHostCoherent)) {
				// Setup the range 
				auto range = vk::MappedMemoryRange()
					.setMemory(target.memory_handle())
					.setOffset(0)
					.setSize(bufferSize);
				// Flush the range
				cgb::context().logical_device().flushMappedMemoryRanges(1, &range);
			}
			cgb::context().logical_device().unmapMemory(target.memory_handle());
			// TODO: Handle has_flag(memProps, vk::MemoryPropertyFlagBits::eHostCached) case

			// No need to sync, so.. don't sync!
			return; // TODO: This should be okay, is it?
		}

		// #2: Otherwise, it must be on the GPU-SIDE!
		else {
			aSyncHandler.set_queue_hint(cgb::context().transfer_queue());
			
			assert(cgb::has_flag(memProps, vk::MemoryPropertyFlagBits::eDeviceLocal));

			// We have to create a (somewhat temporary) staging buffer and transfer it to the GPU
			// "somewhat temporary" means that it can not be deleted in this function, but only
			//						after the transfer operation has completed => handle via sync
			auto stagingBuffer = create_and_fill(
				generic_buffer_meta::create_from_size(target.size()),
				cgb::memory_usage::host_coherent, 
				pData, 
				sync::not_required(), //< It's host coherent memory => no sync will be required
				vk::BufferUsageFlagBits::eTransferSrc);

			auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
			// Sync before:
			aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::transfer, read_memory_access{memory_access::transfer_read_access});

			// Operation:
			auto copyRegion = vk::BufferCopy{}
				.setSrcOffset(0u)
				.setDstOffset(0u)
				.setSize(bufferSize);
			commandBuffer.handle().copyBuffer(stagingBuffer->buffer_handle(), target.buffer_handle(), { copyRegion });

			// Sync after:
			aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::transfer, write_memory_access{memory_access::transfer_write_access});

			// Take care of the lifetime handling of the stagingBuffer, it might still be in use:
			commandBuffer.set_custom_deleter([
				lOwnedStagingBuffer{ std::move(stagingBuffer) }
			]() { /* Nothing to do here, the buffers' destructors will do the cleanup, the lambda is just storing it. */ });
			
			// Finish him:
			aSyncHandler.submit_and_sync();			
		}
	}

	// For convenience:
	inline void fill(const generic_buffer_t& target, const void* pData,			sync aSyncHandler = sync::wait_idle())	{ fill<generic_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	inline void fill(const uniform_buffer_t& target, const void* pData,			sync aSyncHandler = sync::wait_idle())	{ fill<uniform_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	inline void fill(const uniform_texel_buffer_t& target, const void* pData,	sync aSyncHandler = sync::wait_idle())	{ fill<uniform_texel_buffer_meta>(target, pData,	std::move(aSyncHandler)); }
	inline void fill(const storage_buffer_t& target, const void* pData,			sync aSyncHandler = sync::wait_idle())	{ fill<storage_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	inline void fill(const storage_texel_buffer_t& target, const void* pData,	sync aSyncHandler = sync::wait_idle())	{ fill<storage_texel_buffer_meta>(target, pData,	std::move(aSyncHandler)); }
	inline void fill(const vertex_buffer_t& target, const void* pData,			sync aSyncHandler = sync::wait_idle())	{ fill<vertex_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	inline void fill(const index_buffer_t& target, const void* pData,			sync aSyncHandler = sync::wait_idle())	{ fill<index_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	inline void fill(const instance_buffer_t& target, const void* pData,		sync aSyncHandler = sync::wait_idle())	{ fill<instance_buffer_meta>(target, pData,			std::move(aSyncHandler)); }


	// CREATE 
	template <typename Meta>
	cgb::owning_resource<buffer_t<Meta>> create(
		Meta pConfig,
		cgb::memory_usage pMemoryUsage,
		vk::BufferUsageFlags pUsage = vk::BufferUsageFlags())
	{
		auto bufferSize = pConfig.total_size();
		std::optional<vk::DescriptorType> descriptorType = {};
		vk::MemoryPropertyFlags memoryFlags;

		// We've got two major branches here: 
		// 1) Memory will stay on the host and there will be no dedicated memory on the device
		// 2) Memory will be transfered to the device. (Only in this case, we'll need to make use of sync.)
		switch (pMemoryUsage)
		{
		case cgb::memory_usage::host_visible:
			memoryFlags = vk::MemoryPropertyFlagBits::eHostVisible;
			break;
		case cgb::memory_usage::host_coherent:
			memoryFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
			break;
		case cgb::memory_usage::host_cached:
			memoryFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCached;
			break;
		case cgb::memory_usage::device:
			memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
			pUsage |= vk::BufferUsageFlagBits::eTransferDst; // We have to transfer data into it, because we are inside of `cgb::create_and_fill`
			break;
		case cgb::memory_usage::device_readback:
			memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
			pUsage |= vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc;
			break;
		case cgb::memory_usage::device_protected:
			memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eProtected;
			pUsage |= vk::BufferUsageFlagBits::eTransferDst;
			break;
		}

		// TODO: generic_buffer_meta not supported
		if constexpr (std::is_same_v<Meta, cgb::uniform_buffer_meta>) {
			pUsage |= vk::BufferUsageFlagBits::eUniformBuffer;
			descriptorType = vk::DescriptorType::eUniformBuffer;
		}
		else if constexpr (std::is_same_v<Meta, cgb::uniform_texel_buffer_meta>) {
			pUsage |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
			descriptorType = vk::DescriptorType::eUniformTexelBuffer;
		}
		else if constexpr (std::is_same_v<Meta, cgb::storage_buffer_meta>) {
			pUsage |= vk::BufferUsageFlagBits::eStorageBuffer;
			descriptorType = vk::DescriptorType::eStorageBuffer;
		}
		else if constexpr (std::is_same_v<Meta, cgb::storage_texel_buffer_meta>) {
			pUsage |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
			descriptorType = vk::DescriptorType::eStorageTexelBuffer;
		}
		else if constexpr (std::is_same_v<Meta, cgb::vertex_buffer_meta>) {
			pUsage |= vk::BufferUsageFlagBits::eVertexBuffer;
			descriptorType = vk::DescriptorType::eUniformBuffer; // TODO: Does this make sense? Or is this maybe not applicable at all for vertex buffers?
		}
		else if constexpr (std::is_same_v<Meta, cgb::index_buffer_meta>) {
			pUsage |= vk::BufferUsageFlagBits::eIndexBuffer;
			descriptorType = vk::DescriptorType::eUniformBuffer; // TODO: Does this make sense? Or is this maybe not applicable at all for index buffers?
		}

		// Create buffer here to make use of named return value optimization.
		// How it will be filled depends on where the memory is located at.
		return cgb::create(pConfig, pUsage, memoryFlags, descriptorType);
	}

	///** Create multiple buffers */
	//template <typename Meta>
	//auto create_multiple(
	//	int pNumBuffers,
	//	Meta pConfig,
	//	cgb::memory_usage pMemoryUsage,
	//	vk::BufferUsageFlags pUsage = vk::BufferUsageFlags())
	//{
	//	std::vector<buffer_t<Meta>> bs;
	//	bs.reserve(pNumBuffers);
	//	for (int i = 0; i < pNumBuffers; ++i) {
	//		bs.push_back(create(pConfig, pMemoryUsage, pUsage));
	//	}
	//	return bs;
	//}

	// CREATE AND FILL
	template <typename Meta>
	cgb::owning_resource<buffer_t<Meta>> create_and_fill(
		Meta pConfig, 
		cgb::memory_usage pMemoryUsage, 
		const void* pData, 
		sync aSyncHandler = cgb::sync::wait_idle(),
		vk::BufferUsageFlags pUsage = vk::BufferUsageFlags())
	{
		cgb::owning_resource<buffer_t<Meta>> result = create(pConfig, pMemoryUsage, pUsage);
		fill(static_cast<const cgb::buffer_t<Meta>&>(result), pData, std::move(aSyncHandler));
		return std::move(result);
	}


	
	template <typename Meta>
	void read(
		const cgb::buffer_t<Meta>& _Source,
		void* _Data,
		sync aSyncHandler
	)
	{
		auto bufferSize = static_cast<vk::DeviceSize>(_Source.size());
		auto memProps = _Source.memory_properties();

		// #1: Is our memory accessible on the CPU-SIDE? 
		if (cgb::has_flag(memProps, vk::MemoryPropertyFlagBits::eHostVisible)) {
			
			const void* mapped = cgb::context().logical_device().mapMemory(_Source.memory_handle(), 0, bufferSize);
			if (!cgb::has_flag(memProps, vk::MemoryPropertyFlagBits::eHostCoherent)) {
				// Setup the range 
				auto range = vk::MappedMemoryRange()
					.setMemory(_Source.memory_handle())
					.setOffset(0)
					.setSize(bufferSize);
				// Flush the range
				cgb::context().logical_device().invalidateMappedMemoryRanges(1, &range); // TODO: Test this! (should be okay, but double-check against spec.: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkInvalidateMappedMemoryRanges.html)
			}
			memcpy(_Data, mapped, bufferSize);
			cgb::context().logical_device().unmapMemory(_Source.memory_handle());
			// TODO: Handle has_flag(memProps, vk::MemoryPropertyFlagBits::eHostCached) case
		}

		// #2: Otherwise, it must be on the GPU-SIDE!
		else {
			assert(cgb::has_flag(memProps, vk::MemoryPropertyFlagBits::eDeviceLocal));

			aSyncHandler.set_queue_hint(cgb::context().transfer_queue());

			// We have to create a (somewhat temporary) staging buffer and transfer it to the GPU
			// "somewhat temporary" means that it can not be deleted in this function, but only
			//						after the transfer operation has completed => handle via cgb::sync!
			auto stagingBuffer = create(
				generic_buffer_meta::create_from_size(_Source.size()),
				cgb::memory_usage::host_coherent);
			// TODO: Creating a staging buffer in every read()-call is probably not optimal. => Think about alternative ways!

			// TODO: What about queue ownership?! If not the device_queue_selection_strategy::prefer_everything_on_single_queue strategy is being applied, it could very well be that this fails.
			auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
			// Sync before:
			aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::transfer, read_memory_access{memory_access::transfer_read_access});

			// Operation:
			auto copyRegion = vk::BufferCopy{}
				.setSrcOffset(0u)
				.setDstOffset(0u)
				.setSize(bufferSize);
			commandBuffer.handle().copyBuffer(_Source.buffer_handle(), stagingBuffer->buffer_handle(), { copyRegion });

			// Sync after:
			aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::transfer, write_memory_access{memory_access::transfer_write_access});

			// Take care of the stagingBuffer's lifetime handling and also of reading the data for this branch:
			commandBuffer.set_custom_deleter([ 
				lOwnedStagingBuffer{ std::move(stagingBuffer) }, 
				_Data
			]() {
				read(lOwnedStagingBuffer, _Data, sync::not_required()); // TODO: not sure about that sync
			});

			// Finish him:
			aSyncHandler.submit_and_sync();
		}
	}

	inline void read(const generic_buffer_t& target, void* pData,		sync aSyncHandler)	{ read<generic_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	inline void read(const uniform_buffer_t& target, void* pData,		sync aSyncHandler)	{ read<uniform_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	inline void read(const uniform_texel_buffer_t& target, void* pData,	sync aSyncHandler)	{ read<uniform_texel_buffer_meta>(target, pData,	std::move(aSyncHandler)); }
	inline void read(const storage_buffer_t& target, void* pData,		sync aSyncHandler)	{ read<storage_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	inline void read(const storage_texel_buffer_t& target, void* pData,	sync aSyncHandler)	{ read<storage_texel_buffer_meta>(target, pData,	std::move(aSyncHandler)); }
	inline void read(const vertex_buffer_t& target, void* pData,		sync aSyncHandler)	{ read<vertex_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	inline void read(const index_buffer_t& target, void* pData,			sync aSyncHandler)	{ read<index_buffer_meta>(target, pData,			std::move(aSyncHandler)); }
	inline void read(const instance_buffer_t& target, void* pData,		sync aSyncHandler)	{ read<instance_buffer_meta>(target, pData,			std::move(aSyncHandler)); }

	// Convenience overload:
	// TODO: Support collection types and resize them according to memory requirements!
	template <typename Meta, typename Ret>
	Ret read(
		const cgb::buffer_t<Meta>& _Source,
		sync aSyncHandler
	)
	{
		auto memProps = _Source.memory_properties();
		if (!cgb::has_flag(memProps, vk::MemoryPropertyFlagBits::eHostVisible)) {
			throw std::runtime_error("This cgb::read overload can only be used with host-visible buffers. Use cgb::void read(const cgb::buffer_t<Meta>& _Source, void* _Data, sync aSyncHandler) instead!");
		}
		Ret result;
		read(_Source, static_cast<void*>(&result), std::move(aSyncHandler));
		return result;
	}

	template <typename Ret> Ret read(const generic_buffer_t& target,		sync aSyncHandler)	{ return read<generic_buffer_meta, Ret>(target,		  std::move(aSyncHandler)); }
	template <typename Ret> Ret read(const uniform_buffer_t& target,		sync aSyncHandler)	{ return read<uniform_buffer_meta, Ret>(target,		  std::move(aSyncHandler)); }
	template <typename Ret> Ret read(const uniform_texel_buffer_t& target,	sync aSyncHandler)	{ return read<uniform_texel_buffer_meta, Ret>(target, std::move(aSyncHandler)); }
	template <typename Ret> Ret read(const storage_buffer_t& target,		sync aSyncHandler)	{ return read<storage_buffer_meta, Ret>(target,		  std::move(aSyncHandler)); }
	template <typename Ret> Ret read(const storage_texel_buffer_t& target,	sync aSyncHandler)	{ return read<storage_texel_buffer_meta, Ret>(target, std::move(aSyncHandler)); }
	template <typename Ret> Ret read(const vertex_buffer_t& target,			sync aSyncHandler)	{ return read<vertex_buffer_meta, Ret>(target,		  std::move(aSyncHandler)); }
	template <typename Ret> Ret read(const index_buffer_t& target,			sync aSyncHandler)	{ return read<index_buffer_meta, Ret>(target,		  std::move(aSyncHandler)); }
	template <typename Ret> Ret read(const instance_buffer_t& target,		sync aSyncHandler)	{ return read<instance_buffer_meta, Ret>(target,	  std::move(aSyncHandler)); }

}
