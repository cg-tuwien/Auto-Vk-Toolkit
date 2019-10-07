#pragma once

namespace cgb
{
	class top_level_acceleration_structure_t
	{
		template <typename T>
		friend void finish_acceleration_structure_creation(T& result, cgb::context_specific_function<void(T&)> _AlterConfigBeforeMemoryAlloc);

	public:
		top_level_acceleration_structure_t() = default;
		top_level_acceleration_structure_t(const top_level_acceleration_structure_t&) = delete;
		top_level_acceleration_structure_t(top_level_acceleration_structure_t&&) = default;
		top_level_acceleration_structure_t& operator=(const top_level_acceleration_structure_t&) = delete;
		top_level_acceleration_structure_t& operator=(top_level_acceleration_structure_t&&) = default;
		~top_level_acceleration_structure_t() = default;

		const auto& config() const { return mAccStructureInfo; }
		const auto& acceleration_structure_handle() const { return mAccStructure.get(); }
		const auto* acceleration_structure_handle_addr() const { return &mAccStructure.get(); }
		const auto& memory_handle() const { return mMemory.get(); }
		const auto* memory_handle_addr() const { return &mMemory.get(); }
		auto device_handle() const { return mDeviceHandle; }

		size_t required_acceleration_structure_size() const { return static_cast<size_t>(mMemoryRequirementsForAccelerationStructure.memoryRequirements.size); }
		size_t required_scratch_buffer_build_size() const { return static_cast<size_t>(mMemoryRequirementsForBuildScratchBuffer.memoryRequirements.size); }
		size_t required_scratch_buffer_update_size() const { return static_cast<size_t>(mMemoryRequirementsForScratchBufferUpdate.memoryRequirements.size); }

		const auto& descriptor_info() const
		{
			mDescriptorInfo = vk::WriteDescriptorSetAccelerationStructureNV{}
				.setAccelerationStructureCount(1u)
				.setPAccelerationStructures(acceleration_structure_handle_addr());
			return mDescriptorInfo;
		}

		auto descriptor_type() const			{ return vk::DescriptorType::eAccelerationStructureNV; } 

		static owning_resource<top_level_acceleration_structure_t> create(uint32_t _InstanceCount, bool _AllowUpdates = true, cgb::context_specific_function<void(top_level_acceleration_structure_t&)> _AlterConfigBeforeCreation = {}, cgb::context_specific_function<void(top_level_acceleration_structure_t&)> _AlterConfigBeforeMemoryAlloc = {});

		void build(const std::vector<geometry_instance>& _GeometryInstances, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {}, std::vector<semaphore> _WaitSemaphores = {}, std::optional<std::reference_wrapper<const generic_buffer_t>> _ScratchBuffer = {});
		void update(const std::vector<geometry_instance>& _GeometryInstances, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {}, std::vector<semaphore> _WaitSemaphores = {}, std::optional<std::reference_wrapper<const generic_buffer_t>> _ScratchBuffer = {});
		
	private:
		enum struct tlas_action { build, update };
		void build_or_update(const std::vector<geometry_instance>& _GeometryInstances, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler, std::vector<semaphore> _WaitSemaphores, std::optional<std::reference_wrapper<const generic_buffer_t>> _ScratchBuffer, tlas_action _BuildAction);
		const generic_buffer_t& get_and_possibly_create_scratch_buffer();
		
		vk::MemoryRequirements2KHR mMemoryRequirementsForAccelerationStructure;
		vk::MemoryRequirements2KHR mMemoryRequirementsForBuildScratchBuffer;
		vk::MemoryRequirements2KHR mMemoryRequirementsForScratchBufferUpdate;
		vk::MemoryAllocateInfo mMemoryAllocateInfo;
		vk::UniqueDeviceMemory mMemory;

		vk::AccelerationStructureInfoNV mAccStructureInfo;
		vk::ResultValueType<vk::UniqueHandle<vk::AccelerationStructureNV, vk::DispatchLoaderDynamic>>::type mAccStructure;
		uint64_t mDeviceHandle;

		std::optional<generic_buffer> mScratchBuffer;
		
		mutable vk::WriteDescriptorSetAccelerationStructureNV mDescriptorInfo;

		context_tracker<top_level_acceleration_structure_t> mTracker;
	};

	using top_level_acceleration_structure = owning_resource<top_level_acceleration_structure_t>;
}
