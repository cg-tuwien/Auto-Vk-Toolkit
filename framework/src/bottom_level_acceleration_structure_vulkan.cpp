#include <cg_base.hpp>

namespace cgb
{
	owning_resource<bottom_level_acceleration_structure_t> bottom_level_acceleration_structure_t::create(std::vector<std::tuple<vertex_buffer, index_buffer>> aGeometryDescriptions, bool aAllowUpdates, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> aAlterConfigBeforeCreation, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> aAlterConfigBeforeMemoryAlloc)
	{
		bottom_level_acceleration_structure_t result;
		result.mVertexBuffers.reserve(aGeometryDescriptions.size());
		result.mIndexBuffers.reserve(aGeometryDescriptions.size());
		result.mGeometries.reserve(aGeometryDescriptions.size());

		// 1. Gather all geometry descriptions and create vk::GeometryTypeNV entries:
		for (auto& tpl : aGeometryDescriptions) {
			// Perform two sanity checks, because we really need the member descriptions to know where to find the positions.
			// 1st check:
			if (std::get<vertex_buffer>(tpl)->meta_data().member_descriptions().size() == 0) {
				throw std::runtime_error("cgb::vertex_buffers passed to bottom_level_acceleration_structure_t::create must have a member_description for their positions element in their meta data.");
			}

			// Store the data and assemble vk::GeometryNV info:
			result.mVertexBuffers.push_back(std::move(std::get<vertex_buffer>(tpl)));
			// Find member representing the positions, and...
			auto posMember = std::find_if(
				std::begin(result.mVertexBuffers.back()->meta_data().member_descriptions()), 
				std::end(result.mVertexBuffers.back()->meta_data().member_descriptions()), 
				[](const buffer_element_member_meta& md) {
					return md.mContent == content_description::position;
				});
			// ... perform 2nd check:
			if (posMember == std::end(result.mVertexBuffers.back()->meta_data().member_descriptions())) {
				throw std::runtime_error("cgb::vertex_buffers passed to bottom_level_acceleration_structure_t::create has no member which represents positions.");
			}
			result.mIndexBuffers.push_back(std::move(std::get<index_buffer>(tpl)));

			result.mGeometries.emplace_back()
				.setGeometryType(vk::GeometryTypeNV::eTriangles)
				.setGeometry(vk::GeometryDataNV{}
					.setTriangles(vk::GeometryTrianglesNV{}
						.setVertexData(result.mVertexBuffers.back()->buffer_handle())
						.setVertexOffset(posMember->mOffset)
						.setVertexCount(static_cast<uint32_t>(result.mVertexBuffers.back()->meta_data().num_elements()))
						.setVertexStride(result.mVertexBuffers.back()->meta_data().sizeof_one_element())
						.setVertexFormat(posMember->mFormat.mFormat)
						.setIndexData(result.mIndexBuffers.back()->buffer_handle())
						.setIndexOffset(0) // TODO: In which cases might this not be zero?
						.setIndexCount(static_cast<uint32_t>(result.mIndexBuffers.back()->meta_data().num_elements()))
						.setIndexType(to_vk_index_type(result.mIndexBuffers.back()->meta_data().sizeof_one_element()))
						// transformData is a buffer containing optional reference to an array of 32-bit floats 
						// representing a 3x4 row major affine transformation matrix for this geometry.
						.setTransformData(nullptr)
						// transformOffset is the offset in bytes in transformData of the transform 
						// information described above.
						.setTransformOffset(0))
						// TODO: Support transformData and transformOffset
				)
				.setFlags(vk::GeometryFlagBitsNV::eOpaque); // TODO: Support the other flag as well!
		} // for each geometry description

		// 2. Assemble info about the BOTTOM LEVEL acceleration structure and the set its geometry
		result.mAccStructureInfo = vk::AccelerationStructureInfoNV{}
			.setType(vk::AccelerationStructureTypeNV::eBottomLevel)
			.setFlags(aAllowUpdates 
					  ? vk::BuildAccelerationStructureFlagBitsNV::eAllowUpdate | vk::BuildAccelerationStructureFlagBitsNV::ePreferFastBuild 
					  : vk::BuildAccelerationStructureFlagBitsNV::ePreferFastTrace) // TODO: Support flags
			.setInstanceCount(0u)
			.setGeometryCount(static_cast<uint32_t>(result.mGeometries.size()))
			.setPGeometries(result.mGeometries.data());

		// 3. Maybe alter the config?
		if (aAlterConfigBeforeCreation.mFunction) {
			aAlterConfigBeforeCreation.mFunction(result);
		}

		// 4. Create the create info
		auto createInfo = vk::AccelerationStructureCreateInfoNV{}
			.setCompactedSize(0)
			.setInfo(result.mAccStructureInfo);
		// 5. Create it
		result.mAccStructure = context().logical_device().createAccelerationStructureNVUnique(createInfo, nullptr, context().dynamic_dispatch());

		// Steps 6. to 11. in here:
		finish_acceleration_structure_creation(result, std::move(aAlterConfigBeforeMemoryAlloc));
		
		result.mTracker.setTrackee(result);
		return result;
	}

	owning_resource<bottom_level_acceleration_structure_t> bottom_level_acceleration_structure_t::create(std::vector<cgb::aabb> aBoundingBoxes, bool aAllowUpdates, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> aAlterConfigBeforeCreation, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> aAlterConfigBeforeMemoryAlloc)
	{
		bottom_level_acceleration_structure_t result;

		result.mAabbBuffer = cgb::create_and_fill(
			cgb::generic_buffer_meta::create_from_data(aBoundingBoxes),
			memory_usage::host_coherent,
			aBoundingBoxes.data(),
			sync::not_required(),
			vk::BufferUsageFlagBits::eRayTracingNV // TODO: Abstract flags
		);
		
		// 1. Gather all geometry descriptions and create vk::GeometryTypeNV entries:
		for (auto& tpl : aBoundingBoxes) {
			result.mGeometries.emplace_back()
				.setGeometryType(vk::GeometryTypeNV::eAabbs)
				.setGeometry(vk::GeometryDataNV{}
					.setAabbs(vk::GeometryAABBNV{}
						.setAabbData(result.mAabbBuffer.value()->buffer_handle())
						.setNumAABBs(static_cast<uint32_t>(aBoundingBoxes.size()))
						.setStride(sizeof(cgb::aabb))
						.setOffset(0)
					)
				)
				.setFlags(vk::GeometryFlagBitsNV::eOpaque); // TODO: Support the other flag as well!
		} // for each geometry description

		// 2. Assemble info about the BOTTOM LEVEL acceleration structure and the set its geometry
		result.mAccStructureInfo = vk::AccelerationStructureInfoNV{}
			.setType(vk::AccelerationStructureTypeNV::eBottomLevel)
			.setFlags(aAllowUpdates 
					  ? vk::BuildAccelerationStructureFlagBitsNV::eAllowUpdate | vk::BuildAccelerationStructureFlagBitsNV::ePreferFastBuild 
					  : vk::BuildAccelerationStructureFlagBitsNV::ePreferFastTrace) // TODO: Support flags
			.setInstanceCount(0u)
			.setGeometryCount(static_cast<uint32_t>(result.mGeometries.size()))
			.setPGeometries(result.mGeometries.data());

		// 3. Maybe alter the config?
		if (aAlterConfigBeforeCreation.mFunction) {
			aAlterConfigBeforeCreation.mFunction(result);
		}

		// 4. Create the create info
		auto createInfo = vk::AccelerationStructureCreateInfoNV{}
			.setCompactedSize(0)
			.setInfo(result.mAccStructureInfo);
		// 5. Create it
		result.mAccStructure = context().logical_device().createAccelerationStructureNVUnique(createInfo, nullptr, context().dynamic_dispatch());

		// Steps 6. to 11. in here:
		finish_acceleration_structure_creation(result, std::move(aAlterConfigBeforeMemoryAlloc));
		
		result.mTracker.setTrackee(result);
		return result;
	}
	
	owning_resource<bottom_level_acceleration_structure_t> bottom_level_acceleration_structure_t::create(vertex_buffer aVertexBuffer, index_buffer aIndexBuffer, bool aAllowUpdates, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> aAlterConfigBeforeCreation, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> aAlterConfigBeforeMemoryAlloc)
	{
		return create({ std::forward_as_tuple( std::move(aVertexBuffer), std::move(aIndexBuffer) ) }, aAllowUpdates, std::move(aAlterConfigBeforeCreation), std::move(aAlterConfigBeforeMemoryAlloc));
	}

	const generic_buffer_t& bottom_level_acceleration_structure_t::get_and_possibly_create_scratch_buffer()
	{
		if (!mScratchBuffer.has_value()) {
			mScratchBuffer = cgb::create(
				cgb::generic_buffer_meta::create_from_size(std::max(required_scratch_buffer_build_size(), required_scratch_buffer_update_size())),
				cgb::memory_usage::device,
				vk::BufferUsageFlagBits::eRayTracingNV
			);
		}
		return mScratchBuffer.value();
	}
	
	void bottom_level_acceleration_structure_t::build_or_update(sync aSyncHandler, std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer, blas_action aBuildAction)
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

		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		// Sync before:
		aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::acceleration_structure_build, read_memory_access{memory_access::acceleration_structure_read_access});

		// Operation:
		commandBuffer.handle().buildAccelerationStructureNV(
			config(),
			nullptr, 0,								// no instance data for bottom level AS
			aBuildAction == blas_action::build
				? VK_FALSE 
				: VK_TRUE,							// update <=> VK_TRUE
			acceleration_structure_handle(),		// destination AS
			aBuildAction == blas_action::build		// source AS
				? nullptr 
				: acceleration_structure_handle(),
			scratchBuffer->buffer_handle(), 0,		// scratch buffer + offset
			cgb::context().dynamic_dispatch());

		// Sync after:
		aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::acceleration_structure_build, write_memory_access{memory_access::acceleration_structure_write_access});
		
		// Finish him:
		aSyncHandler.submit_and_sync();
	}

	void bottom_level_acceleration_structure_t::build(sync aSyncHandler, std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer)
	{
		build_or_update(std::move(aSyncHandler), std::move(aScratchBuffer), blas_action::build);
	}
	
	void bottom_level_acceleration_structure_t::update(sync aSyncHandler, std::optional<std::reference_wrapper<const generic_buffer_t>> aScratchBuffer)
	{
		build_or_update(std::move(aSyncHandler), std::move(aScratchBuffer), blas_action::update);
	}
}
