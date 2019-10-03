#include <cg_base.hpp>

namespace cgb
{
	geometry_instance geometry_instance::create(glm::mat4 _TransformationMatrix)
	{
		// glm::mat4 mTransform;
		// uint32_t mInstanceCustomIndex;
		// uint32_t mMask;
		// size_t mInstanceOffset;
		// vk::GeometryInstanceFlagsNV mFlags;
		// uint64_t mAccelerationStructureDeviceHandle;
		return geometry_instance
		{
			std::move(_TransformationMatrix),
			0,
			0xff,
			0,
			vk::GeometryInstanceFlagsNV(),
			0,
		};
	}

	geometry_instance& geometry_instance::set_transform(glm::mat4 _TransformationMatrix)
	{
		mTransform = std::move(_TransformationMatrix);
		return *this;
	}

	geometry_instance& geometry_instance::set_custom_index(uint32_t _CustomIndex)
	{
		mInstanceCustomIndex = _CustomIndex;
		return *this;
	}

	geometry_instance& geometry_instance::set_mask(uint32_t _Mask)
	{
		mMask = _Mask;
		return *this;
	}

	geometry_instance& geometry_instance::set_instance_offset(size_t _Offset)
	{
		mInstanceOffset = _Offset;
		return *this;
	}

	geometry_instance& geometry_instance::disable_culling()
	{
		mFlags |= vk::GeometryInstanceFlagBitsNV::eTriangleCullDisable;
		return *this;
	}

	geometry_instance& geometry_instance::define_front_faces_to_be_counter_clockwise()
	{
		mFlags |= vk::GeometryInstanceFlagBitsNV::eTriangleFrontCounterclockwise;
		return *this;
	}

	geometry_instance& geometry_instance::force_opaque()
	{
		mFlags |= vk::GeometryInstanceFlagBitsNV::eForceOpaque;
		return *this;
	}

	geometry_instance& geometry_instance::force_non_opaque()
	{
		mFlags |= vk::GeometryInstanceFlagBitsNV::eForceNoOpaque;
		return *this;
	}


	owning_resource<bottom_level_acceleration_structure_t> bottom_level_acceleration_structure_t::create(std::vector<std::tuple<vertex_buffer, index_buffer>> _GeometryDescriptions, bool _AllowUpdates, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> _AlterConfigBeforeCreation, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> _AlterConfigBeforeMemoryAlloc)
	{
		bottom_level_acceleration_structure_t result;
		result.mVertexBuffers.reserve(_GeometryDescriptions.size());
		result.mIndexBuffers.reserve(_GeometryDescriptions.size());
		result.mGeometries.reserve(_GeometryDescriptions.size());

		// 1. Gather all geometry descriptions and create vk::GeometryTypeNV entries:
		for (auto& tpl : _GeometryDescriptions) {
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


			// Temporary check:
			// TODO: Remove the following line:
			assert(vk::Format::eR32G32B32Sfloat == posMember->mFormat.mFormat);


			result.mGeometries.emplace_back()
				.setGeometryType(vk::GeometryTypeNV::eTriangles)
				.setGeometry(vk::GeometryDataNV{} // TODO: Support AABBs (not only triangles)
					.setTriangles(vk::GeometryTrianglesNV()
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
			.setFlags(_AllowUpdates 
					  ? vk::BuildAccelerationStructureFlagBitsNV::eAllowUpdate | vk::BuildAccelerationStructureFlagBitsNV::ePreferFastBuild 
					  : vk::BuildAccelerationStructureFlagBitsNV::ePreferFastTrace) // TODO: Support flags
			.setInstanceCount(0u)
			.setGeometryCount(static_cast<uint32_t>(result.mGeometries.size()))
			.setPGeometries(result.mGeometries.data());

		// 3. Maybe alter the config?
		if (_AlterConfigBeforeCreation.mFunction) {
			_AlterConfigBeforeCreation.mFunction(result);
		}

		// 4. Create the create info
		auto createInfo = vk::AccelerationStructureCreateInfoNV{}
			.setCompactedSize(0)
			.setInfo(result.mAccStructureInfo);
		// 5. Create it
		result.mAccStructure = context().logical_device().createAccelerationStructureNVUnique(createInfo, nullptr, context().dynamic_dispatch());

		// Steps 6. to 11. in here:
		finish_acceleration_structure_creation(result, std::move(_AlterConfigBeforeMemoryAlloc));
		
		result.mTracker.setTrackee(result);
		return result;
	}

	owning_resource<bottom_level_acceleration_structure_t> bottom_level_acceleration_structure_t::create(vertex_buffer _VertexBuffer, index_buffer _IndexBuffer, bool _AllowUpdates, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> _AlterConfigBeforeCreation, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> _AlterConfigBeforeMemoryAlloc)
	{
		return create({ std::forward_as_tuple( std::move(_VertexBuffer), std::move(_IndexBuffer) ) }, _AllowUpdates, std::move(_AlterConfigBeforeCreation), std::move(_AlterConfigBeforeMemoryAlloc));
	}

	void bottom_level_acceleration_structure_t::add_instance(geometry_instance _Instance)
	{
		mGeometryInstances.push_back(_Instance);
	}

	void bottom_level_acceleration_structure_t::add_instances(std::vector<geometry_instance> _Instances)
	{
		mGeometryInstances.insert(mGeometryInstances.end(), std::begin(_Instances), std::end(_Instances));
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
	
	void bottom_level_acceleration_structure_t::build_or_update(std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler, std::vector<semaphore> _WaitSemaphores, std::optional<std::reference_wrapper<const generic_buffer_t>> _ScratchBuffer, blas_action _BuildAction)
	{
		// Set the _ScratchBuffer parameter to an internal scratch buffer, if none has been passed:
		const generic_buffer_t* scratchBuffer = nullptr;
		if (_ScratchBuffer.has_value()) {
			scratchBuffer = &_ScratchBuffer->get();
		}
		else {
			scratchBuffer = &get_and_possibly_create_scratch_buffer();
		}
		
		auto commandBuffer = cgb::context().transfer_queue().pool().get_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		commandBuffer.begin_recording();

		commandBuffer.handle().buildAccelerationStructureNV(
			config(),
			nullptr, 0,								// no instance data for bottom level AS
			_BuildAction == blas_action::build
				? VK_FALSE 
				: VK_TRUE,							// update <=> VK_TRUE
			acceleration_structure_handle(),		// destination AS
			_BuildAction == blas_action::build		// source AS
				? nullptr 
				: acceleration_structure_handle(),
			scratchBuffer->buffer_handle(), 0,		// scratch buffer + offset
			cgb::context().dynamic_dispatch());
		
		commandBuffer.end_recording();
		
		auto buildCompleteSemaphore = cgb::context().transfer_queue().submit_and_handle_with_semaphore(std::move(commandBuffer), std::move(_WaitSemaphores));
		buildCompleteSemaphore->set_semaphore_wait_stage(vk::PipelineStageFlagBits::eAccelerationStructureBuildNV);
		
		handle_semaphore(std::move(buildCompleteSemaphore), std::move(_SemaphoreHandler));
	}

	void bottom_level_acceleration_structure_t::build(std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler, std::vector<semaphore> _WaitSemaphores, std::optional<std::reference_wrapper<const generic_buffer_t>> _ScratchBuffer)
	{
		build_or_update(std::move(_SemaphoreHandler), std::move(_WaitSemaphores), std::move(_ScratchBuffer), blas_action::build);
	}
	
	void bottom_level_acceleration_structure_t::update(std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler, std::vector<semaphore> _WaitSemaphores, std::optional<std::reference_wrapper<const generic_buffer_t>> _ScratchBuffer)
	{
		build_or_update(std::move(_SemaphoreHandler), std::move(_WaitSemaphores), std::move(_ScratchBuffer), blas_action::update);
	}

	std::vector<VkGeometryInstanceNV> bottom_level_acceleration_structure_t::instance_data_for_top_level_acceleration_structure() const
	{
		if (mGeometryInstances.size() == 0) {
			LOG_WARNING("There are no instances defined for this bottom level acceleration structure.");
		}

		std::vector<VkGeometryInstanceNV> instancesNv;
		instancesNv.reserve(mGeometryInstances.size());
		for (auto& data : mGeometryInstances) {
			auto matrix = glm::transpose(data.mTransform);
			auto& element = instancesNv.emplace_back();
			memcpy(element.transform, glm::value_ptr(matrix), sizeof(element.transform));
			element.instanceCustomIndex = data.mInstanceCustomIndex;
			element.mask = data.mMask;
			element.instanceOffset = data.mInstanceOffset;
			element.flags = static_cast<uint32_t>(data.mFlags);
			element.accelerationStructureHandle = device_handle();
		}
		return instancesNv;
	}
}
