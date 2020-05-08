#pragma once

namespace cgb
{	
	// Helper function used in both, bottom_level_acceleration_structure_t::create and top_level_acceleration_structure_t::create
	template <typename T>
	void finish_acceleration_structure_creation(T& result, cgb::context_specific_function<void(T&)> aAlterConfigBeforeMemoryAlloc) noexcept
	{
		// ------------- Memory ------------
		// 5. Query memory requirements
		{
			auto memReqInfo = vk::AccelerationStructureMemoryRequirementsInfoKHR{}
				.setType(vk::AccelerationStructureMemoryRequirementsTypeKHR::eObject)
				.setBuildType(vk::AccelerationStructureBuildTypeKHR::eDevice) // TODO: support Host builds
				.setAccelerationStructure(result.acceleration_structure_handle());
			result.mMemoryRequirementsForAccelerationStructure = cgb::context().logical_device().getAccelerationStructureMemoryRequirementsKHR(memReqInfo, cgb::context().dynamic_dispatch());
		}
		{
			auto memReqInfo = vk::AccelerationStructureMemoryRequirementsInfoKHR{}
				.setType(vk::AccelerationStructureMemoryRequirementsTypeKHR::eBuildScratch)
				.setBuildType(vk::AccelerationStructureBuildTypeKHR::eDevice) // TODO: support Host builds
				.setAccelerationStructure(result.acceleration_structure_handle());
			result.mMemoryRequirementsForBuildScratchBuffer = cgb::context().logical_device().getAccelerationStructureMemoryRequirementsKHR(memReqInfo, cgb::context().dynamic_dispatch());
		}
		{
			auto memReqInfo = vk::AccelerationStructureMemoryRequirementsInfoKHR{}
				.setType(vk::AccelerationStructureMemoryRequirementsTypeKHR::eUpdateScratch)
				.setBuildType(vk::AccelerationStructureBuildTypeKHR::eDevice) // TODO: support Host builds
				.setAccelerationStructure(result.acceleration_structure_handle());
			result.mMemoryRequirementsForScratchBufferUpdate = cgb::context().logical_device().getAccelerationStructureMemoryRequirementsKHR(memReqInfo, cgb::context().dynamic_dispatch());
		}

		// 6. Assemble memory info
		result.mMemoryAllocateInfo = vk::MemoryAllocateInfo{}
			.setAllocationSize(result.mMemoryRequirementsForAccelerationStructure.memoryRequirements.size)
			.setMemoryTypeIndex(cgb::context().find_memory_type_index(
				result.mMemoryRequirementsForAccelerationStructure.memoryRequirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eDeviceLocal)); // TODO: Does it make sense to support other memory locations as eDeviceLocal?

		// 7. Maybe alter the config?
		if (aAlterConfigBeforeMemoryAlloc.mFunction) {
			aAlterConfigBeforeMemoryAlloc.mFunction(result);
		}

		// 8. Allocate the memory
		result.mMemory = cgb::context().logical_device().allocateMemoryUnique(result.mMemoryAllocateInfo);

		// 9. Bind memory to the acceleration structure
		auto memBindInfo = vk::BindAccelerationStructureMemoryInfoKHR{}
			.setAccelerationStructure(result.acceleration_structure_handle())
			.setMemory(result.memory_handle())
			.setMemoryOffset(0) // TODO: support memory offsets
			.setDeviceIndexCount(0) // TODO: What is this?
			.setPDeviceIndices(nullptr);
		cgb::context().logical_device().bindAccelerationStructureMemoryKHR({ memBindInfo }, cgb::context().dynamic_dispatch());

		// 10. Get an "opaque" handle which can be used on the device
		auto addressInfo = vk::AccelerationStructureDeviceAddressInfoKHR{}
			.setAccelerationStructure(result.acceleration_structure_handle());
		result.mDeviceAddress = cgb::context().logical_device().getAccelerationStructureAddressKHR(&addressInfo, cgb::context().dynamic_dispatch());
	}
}
