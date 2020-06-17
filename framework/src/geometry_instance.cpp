#include <cg_base.hpp>

namespace cgb
{
	geometry_instance geometry_instance::create(const bottom_level_acceleration_structure_t& _Blas)
	{
		// glm::mat4 mTransform;
		// uint32_t mInstanceCustomIndex;
		// uint32_t mMask;
		// size_t mInstanceOffset;
		// vk::GeometryInstanceFlagsKHR mFlags;
		// uint64_t mAccelerationStructureDeviceHandle;
		return geometry_instance
		{
			glm::mat4(1.0f),
			0,
			0xff,
			0,
			vk::GeometryInstanceFlagsKHR(),
			_Blas.device_address()
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
		mFlags |= vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable;
		return *this;
	}

	geometry_instance& geometry_instance::define_front_faces_to_be_counter_clockwise()
	{
		mFlags |= vk::GeometryInstanceFlagBitsKHR::eTriangleFrontCounterclockwise;
		return *this;
	}

	geometry_instance& geometry_instance::force_opaque()
	{
		mFlags |= vk::GeometryInstanceFlagBitsKHR::eForceOpaque;
		return *this;
	}

	geometry_instance& geometry_instance::force_non_opaque()
	{
		mFlags |= vk::GeometryInstanceFlagBitsKHR::eForceNoOpaque;
		return *this;
	}

	geometry_instance& geometry_instance::reset_flags()
	{
		mFlags = vk::GeometryInstanceFlagsKHR();
		return *this;
	}

	VkAccelerationStructureInstanceKHR convert_for_gpu_usage(const geometry_instance& aGeomInst)
	{
		VkAccelerationStructureInstanceKHR element;
		auto matrix = glm::transpose(aGeomInst.mTransform);
		memcpy(&element.transform, glm::value_ptr(matrix), sizeof(element.transform));
		element.instanceCustomIndex = aGeomInst.mInstanceCustomIndex;
		element.mask = aGeomInst.mMask;
		element.instanceShaderBindingTableRecordOffset = aGeomInst.mInstanceOffset;
		element.flags = static_cast<uint32_t>(aGeomInst.mFlags);
		element.accelerationStructureReference = aGeomInst.mAccelerationStructureDeviceHandle;
		return element;
	}

	std::vector<VkAccelerationStructureInstanceKHR> convert_for_gpu_usage(const std::vector<geometry_instance>& aGeomInstances)
	{
		if (aGeomInstances.size() == 0) {
			LOG_WARNING("Empty vector of `geometry_instance`s");
		}

		std::vector<VkAccelerationStructureInstanceKHR> instancesGpu;
		instancesGpu.reserve(aGeomInstances.size());
		for (auto& data : aGeomInstances) {
			instancesGpu.emplace_back(convert_for_gpu_usage(data));			
		}
		return instancesGpu;
	}

}
