#include <cg_base.hpp>

namespace cgb
{
	owning_resource<top_level_acceleration_structure_t> top_level_acceleration_structure_t::create(uint32_t _InstanceCount, bool _AllowUpdates, cgb::context_specific_function<void(top_level_acceleration_structure_t&)> _AlterConfigBeforeCreation, cgb::context_specific_function<void(top_level_acceleration_structure_t&)> _AlterConfigBeforeMemoryAlloc)
	{
		top_level_acceleration_structure_t result;

		// 2. Assemble info about the BOTTOM LEVEL acceleration structure and the set its geometry
		result.mAccStructureInfo = vk::AccelerationStructureInfoNV{}
			.setType(vk::AccelerationStructureTypeNV::eTopLevel)
			.setFlags(_AllowUpdates 
					  ? vk::BuildAccelerationStructureFlagBitsNV::eAllowUpdate | vk::BuildAccelerationStructureFlagBitsNV::ePreferFastBuild 
					  : vk::BuildAccelerationStructureFlagBitsNV::ePreferFastTrace) // TODO: Support flags
			.setInstanceCount(_InstanceCount)
			.setGeometryCount(0u)
			.setPGeometries(nullptr);

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

	const generic_buffer_t& top_level_acceleration_structure_t::get_and_possibly_create_scratch_buffer()
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

	void top_level_acceleration_structure_t::build_or_update(const std::vector<geometry_instance>& _GeometryInstances, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler, std::vector<semaphore> _WaitSemaphores, std::optional<std::reference_wrapper<const generic_buffer_t>> _ScratchBuffer, tlas_action _BuildAction)
	{
		// Set the _ScratchBuffer parameter to an internal scratch buffer, if none has been passed:
		const generic_buffer_t* scratchBuffer = nullptr;
		if (_ScratchBuffer.has_value()) {
			scratchBuffer = &_ScratchBuffer->get();
		}
		else {
			scratchBuffer = &get_and_possibly_create_scratch_buffer();
		}

		std::vector<cgb::VkGeometryInstanceNV> geomInstances = convert_for_gpu_usage(_GeometryInstances);
		
		// TODO: Retain this buffer, don't always create a new one
		auto geomInstBuffer = cgb::create_and_fill(
				cgb::generic_buffer_meta::create_from_data(geomInstances),
				cgb::memory_usage::host_coherent,
				geomInstances.data(),
				nullptr,
				vk::BufferUsageFlagBits::eRayTracingNV
			);
		
		auto commandBuffer = cgb::context().transfer_queue().pool().get_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		commandBuffer.begin_recording();

		commandBuffer.handle().buildAccelerationStructureNV(
			config(),
			geomInstBuffer->buffer_handle(), 0,	    // buffer containing the instance data (only one)
			_BuildAction == tlas_action::build
				? VK_FALSE 
				: VK_TRUE,							// update <=> VK_TRUE
			acceleration_structure_handle(),		// destination AS
			_BuildAction == tlas_action::build		// source AS
				? nullptr 
				: acceleration_structure_handle(),
			scratchBuffer->buffer_handle(), 0,		// scratch buffer + offset
			cgb::context().dynamic_dispatch());
		
		commandBuffer.end_recording();
		
		auto buildCompleteSemaphore = cgb::context().transfer_queue().submit_and_handle_with_semaphore(std::move(commandBuffer), std::move(_WaitSemaphores));
		buildCompleteSemaphore->set_semaphore_wait_stage(vk::PipelineStageFlagBits::eAccelerationStructureBuildNV);
		
		handle_semaphore(std::move(buildCompleteSemaphore), std::move(_SemaphoreHandler));
	}

	void top_level_acceleration_structure_t::build(const std::vector<geometry_instance>& _GeometryInstances, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler, std::vector<semaphore> _WaitSemaphores, std::optional<std::reference_wrapper<const generic_buffer_t>> _ScratchBuffer)
	{
		build_or_update(_GeometryInstances, std::move(_SemaphoreHandler), std::move(_WaitSemaphores), std::move(_ScratchBuffer), tlas_action::build);
	}
	
	void top_level_acceleration_structure_t::update(const std::vector<geometry_instance>& _GeometryInstances, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler, std::vector<semaphore> _WaitSemaphores, std::optional<std::reference_wrapper<const generic_buffer_t>> _ScratchBuffer)
	{
		build_or_update(_GeometryInstances, std::move(_SemaphoreHandler), std::move(_WaitSemaphores), std::move(_ScratchBuffer), tlas_action::update);
	}

}
