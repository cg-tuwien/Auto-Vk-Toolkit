#include <cg_base.hpp>

namespace cgb
{
	owning_resource<top_level_acceleration_structure_t> top_level_acceleration_structure_t::create(uint32_t _InstanceCount, cgb::context_specific_function<void(top_level_acceleration_structure_t&)> _AlterConfigBeforeCreation, cgb::context_specific_function<void(top_level_acceleration_structure_t&)> _AlterConfigBeforeMemoryAlloc)
	{
		top_level_acceleration_structure_t result;

		// 2. Assemble info about the BOTTOM LEVEL acceleration structure and the set its geometry
		result.mAccStructureInfo = vk::AccelerationStructureInfoNV{}
			.setType(vk::AccelerationStructureTypeNV::eTopLevel)
			.setFlags(vk::BuildAccelerationStructureFlagsNV{}) // TODO: Support flags
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
}
