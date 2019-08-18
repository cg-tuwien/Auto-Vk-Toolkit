#pragma once

namespace cgb // ========================== TODO/WIP =================================
{
	struct acceleration_structure_handle
	{
		uint64_t mHandle;
	};

	struct acceleration_structure
	{
		acceleration_structure() noexcept;
		acceleration_structure(acceleration_structure&&) noexcept;
		acceleration_structure& operator=(const acceleration_structure&) = delete;
		acceleration_structure& operator=(acceleration_structure&&) noexcept;
		~acceleration_structure();

		const auto& descriptor_info() const		{ return mDescriptorInfo; }
		const auto& descriptor_type() const		{ return mDescriptorType; }

		static acceleration_structure create_top_level(uint32_t pInstanceCount);
		static acceleration_structure create_bottom_level(const std::vector<vk::GeometryNV>& pGeometries);
		static acceleration_structure create(vk::AccelerationStructureTypeNV pType, const std::vector<vk::GeometryNV>& pGeometries, uint32_t pInstanceCount);

		size_t get_scratch_buffer_size();

		vk::AccelerationStructureInfoNV mAccStructureInfo; // TODO: This is potentially dangerous! The structure stores the pGeometries pointer which might have become invalid. What to do?
		vk::AccelerationStructureNV mAccStructure;
		acceleration_structure_handle mHandle;
		vk::MemoryPropertyFlags mMemoryProperties;
		vk::DeviceMemory mMemory;
		vk::WriteDescriptorSetAccelerationStructureNV mDescriptorInfo; // TODO: set
		vk::DescriptorType mDescriptorType;
		context_tracker<acceleration_structure> mTracker;
	};
}
