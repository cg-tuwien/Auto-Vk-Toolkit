#include <cg_base.hpp>

namespace cgb
{
	owning_resource<top_level_acceleration_structure_t> top_level_acceleration_structure_t::create(uint32_t aInstanceCount, bool aAllowUpdates, cgb::context_specific_function<void(top_level_acceleration_structure_t&)> aAlterConfigBeforeCreation, cgb::context_specific_function<void(top_level_acceleration_structure_t&)> aAlterConfigBeforeMemoryAlloc)
	{
		top_level_acceleration_structure_t result;

		// 2. Assemble info about the BOTTOM LEVEL acceleration structure and the set its geometry
		auto geometryTypeInfo = vk::AccelerationStructureCreateGeometryTypeInfoKHR{}
			.setGeometryType(vk::GeometryTypeKHR::eInstances)
			.setMaxPrimitiveCount(aInstanceCount)
			.setIndexType(vk::IndexType::eNoneKHR)
			.setMaxVertexCount(0u)
			.setVertexFormat(vk::Format::eUndefined)
			.setAllowsTransforms(VK_FALSE);
		
		result.mCreateInfo = vk::AccelerationStructureCreateInfoKHR{}
			.setCompactedSize(0) // If compactedSize is 0 then maxGeometryCount must not be 0
			.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
			.setFlags(aAllowUpdates 
					  ? vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate | vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild
					  : vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace) // TODO: Support flags
			.setMaxGeometryCount(1u) // If type is VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR and compactedSize is 0, maxGeometryCount must be 1
			.setPGeometryInfos(&geometryTypeInfo)
			.setDeviceAddress(VK_NULL_HANDLE);
		
		// 3. Maybe alter the config?
		if (aAlterConfigBeforeCreation.mFunction) {
			aAlterConfigBeforeCreation.mFunction(result);
		}

		// 4. Create it
		result.mAccStructure = context().logical_device().createAccelerationStructureKHRUnique(result.mCreateInfo, nullptr, context().dynamic_dispatch());

		// Steps 5. to 10. in here:
		finish_acceleration_structure_creation(result, std::move(aAlterConfigBeforeMemoryAlloc));

		result.mTracker.setTrackee(result);
		return result;
	}

	const generic_buffer_t& top_level_acceleration_structure_t::get_and_possibly_create_scratch_buffer()
	{
		if (!mScratchBuffer.has_value()) {
			mScratchBuffer = cgb::create(
				cgb::generic_buffer_meta::create_from_size(std::max(required_scratch_buffer_build_size(), required_scratch_buffer_update_size())),
				cgb::memory_usage::device, 
				vk::BufferUsageFlagBits::eRayTracingKHR
			);
		}
		return mScratchBuffer.value();
	}

	std::optional<command_buffer> top_level_acceleration_structure_t::build_or_update(const std::vector<geometry_instance>& aGeometryInstances, sync aSyncHandler, std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer, tlas_action aBuildAction)
	{
		aSyncHandler.set_queue_hint(cgb::context().transfer_queue()); // TODO: better use graphics queue?
		
		// Set the aScratchBuffer parameter to an internal scratch buffer, if none has been passed:
		const generic_buffer_t* scratchBuffer = nullptr;
		if (aScratchBuffer.has_value()) {
			scratchBuffer = &aScratchBuffer->get();
		}
		else {
			scratchBuffer = &get_and_possibly_create_scratch_buffer();
		}

		std::vector<vk::AccelerationStructureGeometryKHR> accStructureGeometries;
		accStructureGeometries.reserve(aGeometryInstances.size());
		std::vector<vk::AccelerationStructureGeometryKHR*> accStructureGeometryPtrs;
		accStructureGeometryPtrs.reserve(aGeometryInstances.size());
		
		std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> buildGeometryInfos;
		buildGeometryInfos.reserve(aGeometryInstances.size()); 
		
		std::vector<vk::AccelerationStructureBuildOffsetInfoKHR> buildOffsetInfos;
		buildOffsetInfos.reserve(aGeometryInstances.size());
		std::vector<vk::AccelerationStructureBuildOffsetInfoKHR*> buildOffsetInfoPtrs; // Points to elements inside buildOffsetInfos... just... because!
		buildOffsetInfoPtrs.reserve(aGeometryInstances.size());
		
		auto geomInstances = convert_for_gpu_usage(aGeometryInstances);
		
		// TODO: Retain this buffer, don't always create a new one
		auto geomInstBuffer = cgb::create_and_fill(
				cgb::generic_buffer_meta::create_from_data(geomInstances),
				cgb::memory_usage::host_coherent,
				geomInstances.data(),
				sync::not_required(),
				vk::BufferUsageFlagBits::eRayTracingKHR | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR // <--- TODO: Which eShaderDeviceAddress*  flag??
			);

		for (auto& gi : aGeometryInstances) {

			auto& asg = accStructureGeometries.emplace_back()
				.setGeometryType(vk::GeometryTypeKHR::eInstances)
				.setGeometry(vk::AccelerationStructureGeometryInstancesDataKHR{}
					.setArrayOfPointers(VK_FALSE) // arrayOfPointers specifies whether data is used as an array of addresses or just an array.
					// TODO: Is this ^ relevant? Probably only for host-builds if the data is structured in "array of pointers"-style?!
					.setData(vk::DeviceOrHostAddressConstKHR{ geomInstBuffer->memory_handle() })
				)
				.setFlags(vk::GeometryFlagsKHR{}); // TODO: Support flags
			
			auto& pAsg = accStructureGeometryPtrs.emplace_back(&asg);
			
			buildGeometryInfos.emplace_back()
				.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
				.setFlags(mCreateInfo.flags)
				.setUpdate(aBuildAction == tlas_action::build ? VK_FALSE : VK_TRUE)
				.setSrcAccelerationStructure(mAccStructure.get()) // TODO: support different src acceleration structure?!
				.setDstAccelerationStructure(mAccStructure.get())
				.setGeometryArrayOfPointers(VK_FALSE)
				.setGeometryCount(1u) // TODO: Correct?
				.setPpGeometries(&pAsg)
				.setScratchData(vk::DeviceOrHostAddressKHR{ scratchBuffer->memory_handle() });

			auto& boi = buildOffsetInfos.emplace_back()
				// For geometries of type VK_GEOMETRY_TYPE_INSTANCES_KHR, primitiveCount is the number of acceleration
				// structures. primitiveCount VkAccelerationStructureInstanceKHR structures are consumed from
				// VkAccelerationStructureGeometryInstancesDataKHR::data, starting at an offset of primitiveOffset.
				.setPrimitiveCount(static_cast<uint32_t>(aGeometryInstances.size())) 
				.setPrimitiveOffset(0u)
				.setFirstVertex(0u)
				.setTransformOffset(0u); // TODO: Support different values for all these parameters?!

			buildOffsetInfoPtrs.emplace_back(&boi);
		}

		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		// Sync before:
		aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::acceleration_structure_build, read_memory_access{memory_access::acceleration_structure_read_access});

		// Operation:
		commandBuffer.handle().buildAccelerationStructureKHR(
			static_cast<uint32_t>(buildGeometryInfos.size()), 
			buildGeometryInfos.data(),
			buildOffsetInfoPtrs.data(),
			cgb::context().dynamic_dispatch()
		);

		// Sync after:
		aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::acceleration_structure_build, write_memory_access{memory_access::acceleration_structure_write_access});

		return aSyncHandler.submit_and_sync();
	}

	void top_level_acceleration_structure_t::build(const std::vector<geometry_instance>& aGeometryInstances, sync aSyncHandler, std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer)
	{
		build_or_update(aGeometryInstances, std::move(aSyncHandler), std::move(aScratchBuffer), tlas_action::build);
	}
	
	void top_level_acceleration_structure_t::update(const std::vector<geometry_instance>& aGeometryInstances, sync aSyncHandler, std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer)
	{
		build_or_update(aGeometryInstances, std::move(aSyncHandler), std::move(aScratchBuffer), tlas_action::update);
	}

}
