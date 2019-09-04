#pragma once

namespace cgb
{
	class bottom_level_acceleration_structure_t
	{
	public:
		bottom_level_acceleration_structure_t() = default;
		bottom_level_acceleration_structure_t(const bottom_level_acceleration_structure_t&) = delete;
		bottom_level_acceleration_structure_t(bottom_level_acceleration_structure_t&&) = default;
		bottom_level_acceleration_structure_t& operator=(const bottom_level_acceleration_structure_t&) = delete;
		bottom_level_acceleration_structure_t& operator=(bottom_level_acceleration_structure_t&&) = default;
		~bottom_level_acceleration_structure_t() = default;

		static owning_resource<bottom_level_acceleration_structure_t> create(std::vector<std::tuple<vertex_buffer, index_buffer>> _GeometryDescriptions, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> _AlterConfigBeforeCreation = {});

	private:
		std::vector<vertex_buffer> mVertexBuffers;
		std::vector<index_buffer> mIndexBuffers;
		std::vector<vk::GeometryNV> mGeometries;

		vk::AccelerationStructureInfoNV mAccStructureInfo; // TODO: This is potentially dangerous! The structure stores the pGeometries pointer which might have become invalid. What to do?
		vk::AccelerationStructureNV mAccStructure;
		vk::UniqueAccelerationStructureNV mHandle;
		vk::MemoryPropertyFlags mMemoryProperties;
		vk::DeviceMemory mMemory;
		vk::WriteDescriptorSetAccelerationStructureNV mDescriptorInfo; // TODO: set
		vk::DescriptorType mDescriptorType;
		context_tracker<acceleration_structure> mTracker;
	};

	using bottom_level_acceleration_structure = owning_resource<bottom_level_acceleration_structure_t>;
}
