#pragma once

namespace cgb
{
	struct geometry_instance
	{
		static geometry_instance create(glm::mat4 _TransformationMatrix = glm::mat4(1.0));

		geometry_instance& set_transform(glm::mat4 _TransformationMatrix);
		geometry_instance& set_custom_index(uint32_t _CustomIndex);
		geometry_instance& set_mask(uint32_t _Mask);
		geometry_instance& set_instance_offset(size_t _Offset);
		geometry_instance& disable_culling();
		geometry_instance& define_front_faces_to_be_counter_clockwise();
		geometry_instance& force_opaque();
		geometry_instance& force_non_opaque();

		glm::mat4 mTransform;
		uint32_t mInstanceCustomIndex;
		uint32_t mMask;
		size_t mInstanceOffset;
		vk::GeometryInstanceFlagsNV mFlags;
		uint64_t mAccelerationStructureDeviceHandle;
	};

	struct VkGeometryInstanceNV
	{
		float		transform[12];
		uint32_t	instanceCustomIndex : 24;
		uint32_t	mask : 8;
		uint32_t	instanceOffset : 24;
		uint32_t	flags : 8;
		uint64_t	accelerationStructureHandle;
	};

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

		const auto& info() const { return mAccStructureInfo; }
		const auto& acceleration_structure_handle() const { return mAccStructure.get(); }
		const auto* acceleration_structure_handle_addr() const { return &mAccStructure.get(); }
		const auto& memory_handle() const { return mMemory.get(); }
		const auto* memory_handle_addr() const { return &mMemory.get(); }
		auto device_handle() const { return mDeviceHandle; }
		auto& instances() { return mGeometryInstances; }
		std::vector<VkGeometryInstanceNV> instance_data_for_top_level_acceleration_structure() const;

		size_t required_acceleration_structure_size() const { return static_cast<size_t>(mMemoryRequirementsForAccelerationStructure.memoryRequirements.size); }
		size_t required_scratch_buffer_build_size() const { return static_cast<size_t>(mMemoryRequirementsForBuildScratchBuffer.memoryRequirements.size); }
		size_t required_scratch_buffer_update_size() const { return static_cast<size_t>(mMemoryRequirementsForScratchBufferUpdate.memoryRequirements.size); }

		static owning_resource<bottom_level_acceleration_structure_t> create(std::vector<std::tuple<vertex_buffer, index_buffer>> _GeometryDescriptions, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> _AlterConfigBeforeCreation = {}, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> _AlterConfigBeforeMemoryAlloc = {});
		static owning_resource<bottom_level_acceleration_structure_t> create(vertex_buffer _VertexBuffer, index_buffer _IndexBuffer, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> _AlterConfigBeforeCreation = {}, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> _AlterConfigBeforeMemoryAlloc = {});

		void add_instance(geometry_instance _Instance);
		void add_instances(std::vector<geometry_instance> _Instances);

	private:
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

		std::vector<geometry_instance> mGeometryInstances;

		context_tracker<bottom_level_acceleration_structure_t> mTracker;
	};

	using bottom_level_acceleration_structure = owning_resource<bottom_level_acceleration_structure_t>;

}
