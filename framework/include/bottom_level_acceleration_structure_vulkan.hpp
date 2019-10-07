#pragma once

namespace cgb
{
	class bottom_level_acceleration_structure_t
	{
		template <typename T>
		friend void finish_acceleration_structure_creation(T& result, cgb::context_specific_function<void(T&)> _AlterConfigBeforeMemoryAlloc);

	public:
		bottom_level_acceleration_structure_t() = default;
		bottom_level_acceleration_structure_t(const bottom_level_acceleration_structure_t&) = delete;
		bottom_level_acceleration_structure_t(bottom_level_acceleration_structure_t&&) = default;
		bottom_level_acceleration_structure_t& operator=(const bottom_level_acceleration_structure_t&) = delete;
		bottom_level_acceleration_structure_t& operator=(bottom_level_acceleration_structure_t&&) = default;
		~bottom_level_acceleration_structure_t() = default;

		const auto& config() const { return mAccStructureInfo; }
		const auto& acceleration_structure_handle() const { return mAccStructure.get(); }
		const auto* acceleration_structure_handle_addr() const { return &mAccStructure.get(); }
		const auto& memory_handle() const { return mMemory.get(); }
		const auto* memory_handle_addr() const { return &mMemory.get(); }
		auto device_handle() const { return mDeviceHandle; }

		size_t required_acceleration_structure_size() const { return static_cast<size_t>(mMemoryRequirementsForAccelerationStructure.memoryRequirements.size); }
		size_t required_scratch_buffer_build_size() const { return static_cast<size_t>(mMemoryRequirementsForBuildScratchBuffer.memoryRequirements.size); }
		size_t required_scratch_buffer_update_size() const { return static_cast<size_t>(mMemoryRequirementsForScratchBufferUpdate.memoryRequirements.size); }

		static owning_resource<bottom_level_acceleration_structure_t> create(std::vector<std::tuple<vertex_buffer, index_buffer>> _GeometryDescriptions, bool _AllowUpdates = true, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> _AlterConfigBeforeCreation = {}, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> _AlterConfigBeforeMemoryAlloc = {});
		static owning_resource<bottom_level_acceleration_structure_t> create(vertex_buffer _VertexBuffer, index_buffer _IndexBuffer, bool _AllowUpdates = true, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> _AlterConfigBeforeCreation = {}, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> _AlterConfigBeforeMemoryAlloc = {});

		void build(std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {}, std::vector<semaphore> _WaitSemaphores = {}, std::optional<std::reference_wrapper<const generic_buffer_t>> _ScratchBuffer = {});
		void update(std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler = {}, std::vector<semaphore> _WaitSemaphores = {}, std::optional<std::reference_wrapper<const generic_buffer_t>> _ScratchBuffer = {});
		
	private:
		enum struct blas_action { build, update };
		void build_or_update(std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler, std::vector<semaphore> _WaitSemaphores, std::optional<std::reference_wrapper<const generic_buffer_t>> _ScratchBuffer, blas_action _BuildAction);
		const generic_buffer_t& get_and_possibly_create_scratch_buffer();
		
		vk::MemoryRequirements2KHR mMemoryRequirementsForAccelerationStructure;
		vk::MemoryRequirements2KHR mMemoryRequirementsForBuildScratchBuffer;
		vk::MemoryRequirements2KHR mMemoryRequirementsForScratchBufferUpdate;
		vk::MemoryAllocateInfo mMemoryAllocateInfo;
		vk::UniqueDeviceMemory mMemory;

		std::vector<vertex_buffer> mVertexBuffers;
		std::vector<index_buffer> mIndexBuffers;
		std::vector<vk::GeometryNV> mGeometries;
		vk::AccelerationStructureInfoNV mAccStructureInfo;
		vk::ResultValueType<vk::UniqueHandle<vk::AccelerationStructureNV, vk::DispatchLoaderDynamic>>::type mAccStructure;
		uint64_t mDeviceHandle;

		std::optional<generic_buffer> mScratchBuffer;
		
		context_tracker<bottom_level_acceleration_structure_t> mTracker;
	};

	using bottom_level_acceleration_structure = owning_resource<bottom_level_acceleration_structure_t>;
}
