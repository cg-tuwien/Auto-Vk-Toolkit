#pragma once

namespace cgb
{	
	// Helper function used in both, bottom_level_acceleration_structure_t::create and top_level_acceleration_structure_t::create
	template <typename T>
	void finish_acceleration_structure_creation(T& result, cgb::context_specific_function<void(T&)> aAlterConfigBeforeMemoryAlloc) noexcept
	{
		// ------------- Memory ------------
		// 6. Query memory requirements
		{
			auto memReqInfo = vk::AccelerationStructureMemoryRequirementsInfoNV{}
				.setAccelerationStructure(result.acceleration_structure_handle())
				.setType(vk::AccelerationStructureMemoryRequirementsTypeNV::eObject);
			result.mMemoryRequirementsForAccelerationStructure = cgb::context().logical_device().getAccelerationStructureMemoryRequirementsNV(memReqInfo, cgb::context().dynamic_dispatch());
		}
		{
			auto memReqInfo = vk::AccelerationStructureMemoryRequirementsInfoNV{}
				.setAccelerationStructure(result.acceleration_structure_handle())
				.setType(vk::AccelerationStructureMemoryRequirementsTypeNV::eBuildScratch);
			result.mMemoryRequirementsForBuildScratchBuffer = cgb::context().logical_device().getAccelerationStructureMemoryRequirementsNV(memReqInfo, cgb::context().dynamic_dispatch());
		}
		{
			auto memReqInfo = vk::AccelerationStructureMemoryRequirementsInfoNV{}
				.setAccelerationStructure(result.acceleration_structure_handle())
				.setType(vk::AccelerationStructureMemoryRequirementsTypeNV::eUpdateScratch);
			result.mMemoryRequirementsForScratchBufferUpdate = cgb::context().logical_device().getAccelerationStructureMemoryRequirementsNV(memReqInfo, cgb::context().dynamic_dispatch());
		}

		// 7. Assemble memory info
		result.mMemoryAllocateInfo = vk::MemoryAllocateInfo{}
			.setAllocationSize(result.mMemoryRequirementsForAccelerationStructure.memoryRequirements.size)
			.setMemoryTypeIndex(cgb::context().find_memory_type_index(
				result.mMemoryRequirementsForAccelerationStructure.memoryRequirements.memoryTypeBits,
				vk::MemoryPropertyFlagBits::eDeviceLocal)); // TODO: Does it make sense to support other memory locations as eDeviceLocal?

		// 8. Maybe alter the config?
		if (aAlterConfigBeforeMemoryAlloc.mFunction) {
			aAlterConfigBeforeMemoryAlloc.mFunction(result);
		}

		// 9. Allocate the memory
		result.mMemory = cgb::context().logical_device().allocateMemoryUnique(result.mMemoryAllocateInfo);

		// 10. Bind memory to the acceleration structure
		auto memBindInfo = vk::BindAccelerationStructureMemoryInfoNV{}
			.setAccelerationStructure(result.acceleration_structure_handle())
			.setMemory(result.memory_handle())
			.setMemoryOffset(0)
			.setDeviceIndexCount(0) // TODO: What is this?
			.setPDeviceIndices(nullptr);
		cgb::context().logical_device().bindAccelerationStructureMemoryNV({ memBindInfo }, cgb::context().dynamic_dispatch());

		// 11. Get an "opaque" handle which can be used on the device
		auto ret = cgb::context().logical_device().getAccelerationStructureHandleNV(result.acceleration_structure_handle(), sizeof(result.mDeviceHandle), &result.mDeviceHandle, context().dynamic_dispatch());
		if (ret != vk::Result::eSuccess) {
			throw cgb::runtime_error("Getting the acceleration structure's opaque device handle failed.");
		}
	}
}
