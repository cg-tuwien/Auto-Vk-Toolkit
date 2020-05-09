#include <cg_base.hpp>

namespace cgb
{
	owning_resource<bottom_level_acceleration_structure_t> bottom_level_acceleration_structure_t::create(std::vector<cgb::acceleration_structure_size_requirements> aGeometryDescriptions, bool aAllowUpdates, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> aAlterConfigBeforeCreation, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> aAlterConfigBeforeMemoryAlloc)
	{
		bottom_level_acceleration_structure_t result;
		result.mGeometryInfos.reserve(aGeometryDescriptions.size());
		
		// 1. Gather all geometry descriptions and create vk::AccelerationStructureCreateGeometryTypeInfoKHR entries:
		for (auto& gd : aGeometryDescriptions) {
			result.mGeometryInfos.emplace_back()
				.setGeometryType(vk::GeometryTypeKHR::eTriangles)
				.setMaxPrimitiveCount(gd.mNumPrimitives)
				.setIndexType(cgb::to_vk_index_type(gd.mIndexTypeSize))
				.setMaxVertexCount(gd.mNumVertices)
				.setVertexFormat(gd.mVertexFormat.mFormat)
				.setAllowsTransforms(VK_FALSE); // TODO: Add support for transforms (allowsTransforms indicates whether transform data can be used by this acceleration structure or not, when geometryType is VK_GEOMETRY_TYPE_TRIANGLES_KHR.)
		} // for each geometry description
		
		// 2. Assemble info about the BOTTOM LEVEL acceleration structure and the set its geometry
		result.mCreateInfo = vk::AccelerationStructureCreateInfoKHR{}
			.setCompactedSize(0) // If compactedSize is 0 then maxGeometryCount must not be 0
			.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
			.setFlags(aAllowUpdates 
					  ? vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate | vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild
					  : vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace) // TODO: Support flags
			.setMaxGeometryCount(static_cast<uint32_t>(result.mGeometryInfos.size()))
			.setPGeometryInfos(result.mGeometryInfos.data())
			.setDeviceAddress(VK_NULL_HANDLE); // TODO: support this (deviceAddress is the device address requested for the acceleration structure if the rayTracingAccelerationStructureCaptureReplay feature is being used.)
		
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

	//owning_resource<bottom_level_acceleration_structure_t> bottom_level_acceleration_structure_t::create(std::vector<cgb::aabb> aBoundingBoxes, bool aAllowUpdates, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> aAlterConfigBeforeCreation, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> aAlterConfigBeforeMemoryAlloc)
	//{
	//	bottom_level_acceleration_structure_t result;

	//	result.mAabbBuffer = cgb::create_and_fill(
	//		cgb::generic_buffer_meta::create_from_data(aBoundingBoxes),
	//		memory_usage::host_coherent,
	//		aBoundingBoxes.data(),
	//		sync::not_required(),
	//		vk::BufferUsageFlagBits::eRayTracingKHR // TODO: Abstract flags
	//	);
	//	
	//	// 1. Gather all geometry descriptions and create vk::GeometryTypeKHR entries:
	//	for (auto& tpl : aBoundingBoxes) {
	//		result.mGeometries.emplace_back()
	//			.setGeometryType(vk::GeometryTypeNV::eAabbs)
	//			.setGeometry(vk::GeometryDataNV{}
	//				.setAabbs(vk::GeometryAABBNV{}
	//					.setAabbData(result.mAabbBuffer.value()->buffer_handle())
	//					.setNumAABBs(static_cast<uint32_t>(aBoundingBoxes.size()))
	//					.setStride(sizeof(cgb::aabb))
	//					.setOffset(0)
	//				)
	//			)
	//			.setFlags(vk::GeometryFlagBitsNV::eOpaque); // TODO: Support the other flag as well!
	//	} // for each geometry description

	//	// 2. Assemble info about the BOTTOM LEVEL acceleration structure and the set its geometry
	//	result.mAccStructureInfo = vk::AccelerationStructureInfoNV{}
	//		.setType(vk::AccelerationStructureTypeNV::eBottomLevel)
	//		.setFlags(aAllowUpdates 
	//				  ? vk::BuildAccelerationStructureFlagBitsNV::eAllowUpdate | vk::BuildAccelerationStructureFlagBitsNV::ePreferFastBuild 
	//				  : vk::BuildAccelerationStructureFlagBitsNV::ePreferFastTrace) // TODO: Support flags
	//		.setInstanceCount(0u)
	//		.setGeometryCount(static_cast<uint32_t>(result.mGeometries.size()))
	//		.setPGeometries(result.mGeometries.data());

	//	// 3. Maybe alter the config?
	//	if (aAlterConfigBeforeCreation.mFunction) {
	//		aAlterConfigBeforeCreation.mFunction(result);
	//	}

	//	// 4. Create the create info
	//	auto createInfo = vk::AccelerationStructureCreateInfoNV{}
	//		.setCompactedSize(0)
	//		.setInfo(result.mAccStructureInfo);
	//	// 5. Create it
	//	result.mAccStructure = context().logical_device().createAccelerationStructureNVUnique(createInfo, nullptr, context().dynamic_dispatch());

	//	// Steps 6. to 11. in here:
	//	finish_acceleration_structure_creation(result, std::move(aAlterConfigBeforeMemoryAlloc));
	//	
	//	result.mTracker.setTrackee(result);
	//	return result;
	//}
	
	const generic_buffer_t& bottom_level_acceleration_structure_t::get_and_possibly_create_scratch_buffer()
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
	
	std::optional<command_buffer> bottom_level_acceleration_structure_t::build_or_update(std::vector<std::tuple<std::reference_wrapper<const cgb::vertex_buffer_t>, std::reference_wrapper<const cgb::index_buffer_t>>> aGeometries, sync aSyncHandler, std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer, blas_action aBuildAction)
	{
		aSyncHandler.set_queue_hint(cgb::context().transfer_queue()); // TODO: Transfer queue okay? Or graphics queue better?
		
		// Set the aScratchBuffer parameter to an internal scratch buffer, if none has been passed:
		const generic_buffer_t* scratchBuffer = nullptr;
		if (aScratchBuffer.has_value()) {
			scratchBuffer = &aScratchBuffer->get();
		}
		else {
			scratchBuffer = &get_and_possibly_create_scratch_buffer();
		}

		std::vector<vk::AccelerationStructureGeometryKHR> accStructureGeometries;
		accStructureGeometries.reserve(aGeometries.size());
		std::vector<vk::AccelerationStructureGeometryKHR*> accStructureGeometryPtrs;
		accStructureGeometryPtrs.reserve(aGeometries.size());
		
		std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> buildGeometryInfos;
		buildGeometryInfos.reserve(aGeometries.size()); 
		
		std::vector<vk::AccelerationStructureBuildOffsetInfoKHR> buildOffsetInfos;
		buildOffsetInfos.reserve(aGeometries.size());
		std::vector<vk::AccelerationStructureBuildOffsetInfoKHR*> buildOffsetInfoPtrs; // Points to elements inside buildOffsetInfos... just... because!
		buildOffsetInfoPtrs.reserve(aGeometries.size());

		for (auto& tpl : aGeometries) {
			auto& vertexBuffer = std::get<std::reference_wrapper<const cgb::vertex_buffer_t>>(tpl).get();
			auto& indexBuffer = std::get<std::reference_wrapper<const cgb::index_buffer_t>>(tpl).get();
			
			if (vertexBuffer.meta_data().member_descriptions().size() == 0) {
				throw cgb::runtime_error("cgb::vertex_buffers passed to acceleration_structure_size_requirements::from_buffers must have a member_description for their positions element in their meta data.");
			}
			// Find member representing the positions, and...
			auto posMember = std::find_if(
				std::begin(vertexBuffer.meta_data().member_descriptions()), 
				std::end(vertexBuffer.meta_data().member_descriptions()), 
				[](const buffer_element_member_meta& md) {
					return md.mContent == content_description::position;
				});
			// ... perform 2nd check:
			if (posMember == std::end(vertexBuffer.meta_data().member_descriptions())) {
				throw cgb::runtime_error("cgb::vertex_buffers passed to acceleration_structure_size_requirements::from_buffers has no member which represents positions.");
			}

			auto& asg = accStructureGeometries.emplace_back()
				.setGeometryType(vk::GeometryTypeKHR::eTriangles)
				.setGeometry(vk::AccelerationStructureGeometryTrianglesDataKHR{}
					.setVertexFormat(posMember->mFormat.mFormat)
					.setVertexData(vk::DeviceOrHostAddressConstKHR{ vertexBuffer.memory_handle() }) // TODO: Support host addresses
					.setVertexStride(static_cast<vk::DeviceSize>(vertexBuffer.meta_data().sizeof_one_element()))
					.setIndexData(vk::DeviceOrHostAddressConstKHR{ indexBuffer.memory_handle() }) // TODO: Support host addresses
					.setTransformData(nullptr)
				)
				.setFlags(vk::GeometryFlagsKHR{}); // TODO: Support flags

			auto& pAsg = accStructureGeometryPtrs.emplace_back(&asg);
			
			buildGeometryInfos.emplace_back()
				.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
				.setFlags(mCreateInfo.flags) // TODO: support individual flags per geometry?
				.setUpdate(aBuildAction == blas_action::build ? VK_FALSE : VK_TRUE)
				.setSrcAccelerationStructure(mAccStructure.get()) // TODO: support different src acceleration structure?!
				.setDstAccelerationStructure(mAccStructure.get())
				.setGeometryArrayOfPointers(VK_FALSE)
				.setGeometryCount(1u) // TODO: Correct?
				.setPpGeometries(&pAsg)
				.setScratchData(vk::DeviceOrHostAddressKHR{ scratchBuffer->memory_handle() });

			auto& boi = buildOffsetInfos.emplace_back()
				.setPrimitiveCount(static_cast<uint32_t>(indexBuffer.meta_data().num_elements()) / 3u)
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
		
		// Finish him:
		return aSyncHandler.submit_and_sync();
	}

	void bottom_level_acceleration_structure_t::build(std::vector<std::tuple<std::reference_wrapper<const cgb::vertex_buffer_t>, std::reference_wrapper<const cgb::index_buffer_t>>> aGeometries, sync aSyncHandler, std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer)
	{
		build_or_update(std::move(aGeometries), std::move(aSyncHandler), std::move(aScratchBuffer), blas_action::build);
	}
	
	void bottom_level_acceleration_structure_t::update(std::vector<std::tuple<std::reference_wrapper<const cgb::vertex_buffer_t>, std::reference_wrapper<const cgb::index_buffer_t>>> aGeometries, sync aSyncHandler, std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer)
	{
		build_or_update(std::move(aGeometries), std::move(aSyncHandler), std::move(aScratchBuffer), blas_action::update);
	}
}
