#pragma once

namespace cgb
{
	class bottom_level_acceleration_structure_t
	{
		template <typename T>
		friend void finish_acceleration_structure_creation(T&, cgb::context_specific_function<void(T&)>) noexcept;

	public:
		bottom_level_acceleration_structure_t() = default;
		bottom_level_acceleration_structure_t(bottom_level_acceleration_structure_t&&) noexcept = default;
		bottom_level_acceleration_structure_t(const bottom_level_acceleration_structure_t&) = delete;
		bottom_level_acceleration_structure_t& operator=(bottom_level_acceleration_structure_t&&) noexcept = default;
		bottom_level_acceleration_structure_t& operator=(const bottom_level_acceleration_structure_t&) = delete;
		~bottom_level_acceleration_structure_t() = default;

		const auto& config() const { return mCreateInfo; }
		const auto& acceleration_structure_handle() const { return mAccStructure.get(); }
		const auto* acceleration_structure_handle_ptr() const { return &mAccStructure.get(); }
		const auto& memory_handle() const { return mMemory.get(); }
		const auto* memory_handle_ptr() const { return &mMemory.get(); }
		auto device_address() const { return mDeviceAddress; }

		size_t required_acceleration_structure_size() const { return static_cast<size_t>(mMemoryRequirementsForAccelerationStructure.memoryRequirements.size); }
		size_t required_scratch_buffer_build_size() const { return static_cast<size_t>(mMemoryRequirementsForBuildScratchBuffer.memoryRequirements.size); }
		size_t required_scratch_buffer_update_size() const { return static_cast<size_t>(mMemoryRequirementsForScratchBufferUpdate.memoryRequirements.size); }

		static owning_resource<bottom_level_acceleration_structure_t> create(std::vector<cgb::acceleration_structure_size_requirements> aGeometryDescriptions, bool aAllowUpdates, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> aAlterConfigBeforeCreation = {}, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> aAlterConfigBeforeMemoryAlloc = {});

		void build(std::vector<std::tuple<std::reference_wrapper<const cgb::vertex_buffer_t>, std::reference_wrapper<const cgb::index_buffer_t>>> aGeometries, sync aSyncHandler = sync::wait_idle(), std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer = {});
		void update(std::vector<std::tuple<std::reference_wrapper<const cgb::vertex_buffer_t>, std::reference_wrapper<const cgb::index_buffer_t>>> aGeometries, sync aSyncHandler = sync::wait_idle(), std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer = {});
		void build(cgb::generic_buffer aBuffer, std::vector<cgb::aabb> aGeometries, sync aSyncHandler = sync::wait_idle(), std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer = {});
		void update(cgb::generic_buffer aBuffer, std::vector<cgb::aabb> aGeometries, sync aSyncHandler = sync::wait_idle(), std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer = {});
		
	private:
		enum struct blas_action { build, update };
		std::optional<command_buffer> build_or_update(std::vector<std::tuple<std::reference_wrapper<const cgb::vertex_buffer_t>, std::reference_wrapper<const cgb::index_buffer_t>>> aGeometries, sync aSyncHandler, std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer, blas_action aBuildAction);
		std::optional<command_buffer> build_or_update(cgb::generic_buffer aBuffer, std::vector<cgb::aabb> aGeometries, sync aSyncHandler, std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer, blas_action aBuildAction);
		const generic_buffer_t& get_and_possibly_create_scratch_buffer();
		
		vk::MemoryRequirements2KHR mMemoryRequirementsForAccelerationStructure;
		vk::MemoryRequirements2KHR mMemoryRequirementsForBuildScratchBuffer;
		vk::MemoryRequirements2KHR mMemoryRequirementsForScratchBufferUpdate;
		vk::MemoryAllocateInfo mMemoryAllocateInfo;
		vk::UniqueDeviceMemory mMemory;

		std::vector<vk::AccelerationStructureCreateGeometryTypeInfoKHR> mGeometryInfos;
		//std::vector<vk::GeometryKHR> mGeometries;
		vk::AccelerationStructureCreateInfoKHR mCreateInfo;
		vk::ResultValueType<vk::UniqueHandle<vk::AccelerationStructureKHR, vk::DispatchLoaderDynamic>>::type mAccStructure;
		vk::DeviceAddress mDeviceAddress;

		std::optional<generic_buffer> mScratchBuffer;

		std::optional<generic_buffer> mAabbBuffer;
		
		context_tracker<bottom_level_acceleration_structure_t> mTracker;
	};

	using bottom_level_acceleration_structure = owning_resource<bottom_level_acceleration_structure_t>;
}
